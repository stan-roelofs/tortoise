#include "torrent_impl.hpp"

#include <cassert>

#include "../util/log.hpp"

namespace
{
	constexpr std::size_t DESIRED_PEERS = 20;
}

namespace tortoise
{
	bool TorrentHandle::IsValid() const
	{
		return !ptr_.expired();
	}

	Metainfo TorrentHandle::GetMetainfo() const
	{
		if (!IsValid())
			throw InvalidHandleException("TorrentHandle is not valid");

		return *ptr_.lock()->GetMetainfo();
	}

	void TorrentHandle::StartDownload()
	{
		if (!IsValid())
			throw InvalidHandleException("TorrentHandle is not valid");

		ptr_.lock()->Start();
	}

	void TorrentHandle::StopDownload()
	{
		if (!IsValid())
			throw InvalidHandleException("TorrentHandle is not valid");

		ptr_.lock()->Stop();
	}

	TorrentHandle::operator bool() const
	{
		return IsValid();
	}

	bool TorrentHandle::operator==(const TorrentHandle& other) const
	{
		return ptr_.lock() == other.ptr_.lock();
	}

	bool TorrentHandle::operator!=(const TorrentHandle& other) const
	{
		return !(*this == other);
	}

	Torrent::Torrent(const TorrentParameters& parameters, Torrent::PeerInfoProvider& peer_info_provider, EventQueue& event_queue)
		: running_(false),
		requested_peers_(false),
		peer_info_provider_(peer_info_provider),
		event_queue_(event_queue),
		metainfo_(std::make_shared<Metainfo>(parameters.metainfo)),
		piece_manager_(metainfo_->pieces.size())
	{
		if (parameters.save_path.empty())
			throw Exception("save_path is empty");

		peer_info_provider_.RegisterTorrent(*this, std::bind(&Torrent::OnNewPeers, this, std::placeholders::_1));
	}

	Torrent::~Torrent()
	{
		Stop();

		std::lock_guard lock(mutex_);
		peer_info_provider_.UnregisterTorrent(*this);
	}

	void Torrent::Run(Torrent& torrent)
	{
		while (torrent.running_)
		{
			torrent.ProcessPeers();

			if (torrent.running_)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	void Torrent::Start()
	{
		std::lock_guard lock(mutex_);

		if (running_)
			return;

		running_ = true;
		thread_ = std::thread(Torrent::Run, std::ref(*this));

		LOG_INFO("Torrent", "Torrent started");
	}

	void Torrent::Stop()
	{
		std::lock_guard lock(mutex_);

		running_ = false;
		if (thread_.joinable())
			thread_.join();

		LOG_INFO("Torrent", "Torrent stopped");
	}

	std::shared_ptr<const Metainfo> Torrent::GetMetainfo() const
	{
		std::lock_guard lock(mutex_);

		return metainfo_;
	}

	PeerId Torrent::GetPeerId() const
	{
		std::lock_guard lock(mutex_);

		return peer_id_;
	}

	void Torrent::OnNewPeers(const std::vector<PeerInfo>& new_peers)
	{
		std::lock_guard lock(mutex_);

		for (const auto& peer_info : new_peers)
		{
			if (std::find(potential_peers_.begin(), potential_peers_.end(), peer_info) != potential_peers_.end())
				return;
			if (std::find_if(peers_.begin(), peers_.end(), [&peer_info](const auto& peer)
				{ return peer.first->GetPeerInfo() == peer_info; }) != peers_.end())
				return;
			potential_peers_.push_back(peer_info);
		}

		requested_peers_ = false;
	}

	void Torrent::ProcessPeers()
	{
		std::lock_guard lock(mutex_);

		if (!requested_peers_ && (potential_peers_.size() + peers_.size()) < DESIRED_PEERS)
		{
			LOG_INFO("Torrent", "Requesting peers");
			requested_peers_ = true;
			peer_info_provider_.RequestPeers(*this, DESIRED_PEERS);
		}

		// Add new peers if we don't have enough and there are still peers in the queue
		// TODO send a tracker request if we need more peers
		while (peers_.size() < DESIRED_PEERS && !potential_peers_.empty())
		{
			PeerInfo peer_info = potential_peers_.front();
			potential_peers_.pop_front();

			LOG_INFO("Torrent", std::format("Pending peer {}", peer_info.ToString()));
			peers_.push_back({ std::make_unique<Peer>(peer_info, metainfo_, peer_id_), PeerStatus::Unknown });
		}

		{
			auto it = peers_.begin();
			while (it != peers_.end())
			{
				const auto status = it->first->Process();
				if (status != it->second)
				{
					it->second = status;
					event_queue_.Push(event::PeerStatusChanged {shared_from_this(), it->first->GetPeerInfo(), status});

					switch (status)
					{
					case PeerStatus::Unknown:
					case PeerStatus::Connecting:
					case PeerStatus::Connected:
						++it;
						break;
					case PeerStatus::Disconnected:
					{
						it = peers_.erase(it);
						break;
					}
					}
				}
			}
		}
	}
}