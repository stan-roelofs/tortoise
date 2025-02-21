#include "torrent_impl.hpp"

#include <cassert>

#include "../util/log.hpp"

namespace
{
	constexpr std::size_t DESIRED_PEERS = 30;
	constexpr std::chrono::seconds PEER_REQUEST_INTERVAL = std::chrono::seconds(600);
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

		return ptr_.lock()->GetMetainfo();
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

	Statistics TorrentHandle::GetStatistics() const
	{
		if (!IsValid())
			throw InvalidHandleException("TorrentHandle is not valid");

		return ptr_.lock()->GetStatistics();
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
		peer_info_provider_(peer_info_provider),
		event_queue_(event_queue),
		metainfo_(std::make_shared<const Metainfo>(parameters.metainfo)),
		piece_manager_(metainfo_),
		upload_speed_(0),
		download_speed_(0)
	{
		if (parameters.save_path.empty())
			throw Exception("save_path is empty");

		peer_info_provider_.RegisterTorrent(*this, std::bind(&Torrent::OnNewPeers, this, std::placeholders::_1));
		piece_manager_.AddListener(this);
	}

	Torrent::~Torrent()
	{
		Stop();

		piece_manager_.RemoveListener(this);

		std::lock_guard lock(mutex_);
		peer_info_provider_.UnregisterTorrent(*this);
	}

	void Torrent::Run(Torrent& torrent)
	{
		while (torrent.running_)
		{
			torrent.ProcessPeers();

			// Update rates...
			torrent.download_speed_ = 0;
			torrent.upload_speed_ = 0;
			for (const auto& peer : torrent.peers_)
			{
				torrent.download_speed_ += peer.first->GetDownloadSpeed();
				torrent.upload_speed_ += peer.first->GetUploadSpeed();
			}

			// if (torrent.running_)
			// std::this_thread::sleep_for(std::chrono::milliseconds(100));
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

	const Metainfo& Torrent::GetMetainfo() const
	{
		return *metainfo_;
	}

	PeerId Torrent::GetPeerId() const
	{
		std::lock_guard lock(mutex_);

		return peer_id_;
	}

	Statistics Torrent::GetStatistics() const
	{
		std::lock_guard lock(mutex_);

		Statistics stats;
		stats.download_rate = download_speed_;
		stats.upload_rate = upload_speed_;
		return stats;
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
	}

	void Torrent::OnPieceDownloaded(std::uint32_t piece_index)
	{
		event_queue_.Push(event::PieceDownloaded{ shared_from_this(), piece_index });
	}

	void Torrent::RequestPeers()
	{
		LOG_INFO("Torrent", "Requesting peers");
		last_peer_request_ = std::chrono::steady_clock::now();
		peer_info_provider_.RequestPeers(*this, DESIRED_PEERS * 2);
	}

	void Torrent::ProcessPeers()
	{
		std::lock_guard lock(mutex_);

		const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		if ((potential_peers_.size() + peers_.size()) < DESIRED_PEERS && now - last_peer_request_ > PEER_REQUEST_INTERVAL)
			RequestPeers();

		// Add new peers if we don't have enough and there are still peers in the queue
		// TODO send a tracker request if we need more peers
		while (peers_.size() < DESIRED_PEERS && !potential_peers_.empty())
		{
			PeerInfo peer_info = potential_peers_.front();
			potential_peers_.pop_front();

			LOG_INFO("Torrent", std::format("Pending peer {}", peer_info.ToString()));
			peers_.push_back({ std::make_unique<Peer>(peer_info, metainfo_, peer_id_, piece_manager_), PeerStatus::Connecting });
			event_queue_.Push(event::PeerStatusChanged{ shared_from_this(), peer_info, PeerStatus::Connecting });
		}

		{
			auto it = peers_.begin();
			while (it != peers_.end())
			{
				const auto status = it->first->Process();
				if (status != it->second)
				{
					it->second = status;
					event_queue_.Push(event::PeerStatusChanged{ shared_from_this(), it->first->GetPeerInfo(), status });
				}

				// TODO: if peer slow or has no pieces we need AND there are potential peers, disconnect and connect to a new peer

				switch (status)
				{
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