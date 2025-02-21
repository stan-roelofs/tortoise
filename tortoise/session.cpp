#include <tortoise/session.hpp>

#include <filesystem>

#include "event_queue.hpp"
#include "torrent/torrent_impl.hpp"
#include "tracker/manager.hpp"

namespace tortoise
{
	class Session::Implementation
	{
	public:
		Implementation(event::Callbacks callbacks) : callbacks_(callbacks)
		{
#define SUBSCRIBE_EVENT(cb, eventtype) \
	if (callbacks_.cb)                 \
		event_queue_.Subscribe<event::eventtype>([this](const auto &event) { callbacks_.cb(event); });

			SUBSCRIBE_EVENT(torrent_added, TorrentAdded);
			SUBSCRIBE_EVENT(peer_status_changed, PeerStatusChanged);
			SUBSCRIBE_EVENT(piece_downloaded, PieceDownloaded);

#undef SUBSCRIBE_EVENT
		}

		~Implementation() = default;

		TorrentHandle AddTorrent(TorrentParameters parameters)
		{
			if (parameters.save_path.empty())
				parameters.save_path = std::filesystem::current_path();

			// TODO check if torrent already exists

			auto torrent = std::make_shared<Torrent>(parameters, tracker_manager_, event_queue_);
			event_queue_.Push(event::TorrentAdded(torrent));

			{
				std::lock_guard lock(mutex_);
				torrents_.push_back(torrent);
			}

			return torrent;
		}

		void RemoveTorrent(TorrentHandle handle)
		{
			if (!handle.IsValid())
				return;

			std::lock_guard lock(mutex_);
			auto it = std::find_if(torrents_.begin(), torrents_.end(), [handle](const auto& torrent)
				{ return TorrentHandle(torrent) == handle; });
			if (it != torrents_.end()) // TODO can this block if we need to close connections etc? may need to implement this in a different way
				torrents_.erase(it);
		}

		void HandleEvents()
		{
			std::lock_guard guard(mutex_);
			event_queue_.Process();
		}

	private:
		tracker::Manager tracker_manager_; // Keep this before torrents_ so it is destroyed after
		std::vector<std::shared_ptr<Torrent>> torrents_;
		EventQueue event_queue_;
		std::mutex mutex_;
		event::Callbacks callbacks_;
	};

	Session::Session(event::Callbacks callbacks) : implementation_(std::make_unique<Implementation>(callbacks))
	{
	}

	Session::~Session() = default;

	TorrentHandle Session::AddTorrent(TorrentParameters params)
	{
		return implementation_->AddTorrent(params);
	}

	void Session::RemoveTorrent(TorrentHandle handle)
	{
		implementation_->RemoveTorrent(handle);
	}

	void Session::HandleEvents()
	{
		implementation_->HandleEvents();
	}
}