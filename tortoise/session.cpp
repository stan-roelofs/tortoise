#include <tortoise/session.hpp>

#include "torrent/torrent_impl.hpp"
#include "tracker/manager.hpp"

namespace tortoise
{
    class Session::Implementation
    {
    public:
        Implementation(EventCallbacks callbacks) : callbacks_(callbacks)
        {
        }

        ~Implementation()
        {
        }

        TorrentHandle AddTorrent(const TorrentParameters &parameters)
        {
            // if (params.save_path.empty())
            //  TODO get default path from settings

            // TODO check if torrent already exists

            TorrentData new_torrent;
            new_torrent.event_queue = std::make_unique<EventQueue>(callbacks_);
            auto torrent = std::make_shared<Torrent>(parameters, tracker_manager_, *new_torrent.event_queue);
            new_torrent.torrent = torrent;
            {
                std::lock_guard lock(mutex_);
                torrents_.push_back(std::move(new_torrent));
            }

            return torrent;
        }

        void RemoveTorrent(TorrentHandle handle)
        {
            if (!handle.IsValid())
                return;

            std::lock_guard lock(mutex_);
            auto it = std::find_if(torrents_.begin(), torrents_.end(), [&](const TorrentData &torrent)
                                   { return TorrentHandle(torrent.torrent) == handle; });
            if (it != torrents_.end()) // TODO can this block if we need to close connections etc? may need to implement this in a different way
                torrents_.erase(it);
        }

        void HandleEvents()
        {
            std::lock_guard guard(mutex_);
            for (auto &torrent : torrents_)
            {
                std::vector<std::unique_ptr<Event>> events = torrent.event_queue->PopEvents();

                for (auto &event : events)
                {
                    Event *e = event.get();

                    // TODO: this is dumb, we shouldnt need to check every event type here

                    // Note: we could use a visitor pattern here, but it is a lot of effort with little benefit.
                    if (dynamic_cast<TorrentAddedEvent *>(e))
                    {
                        callbacks_.torrent_added(torrent.torrent);
                    }
                    else if (auto *peer_status_event = dynamic_cast<PeerStatusChangedEvent *>(e))
                    {
                        callbacks_.peer_status_changed(torrent.torrent, peer_status_event->ip, peer_status_event->port, peer_status_event->status);
                    }
                }
            }
        }

    private:
        struct TorrentData
        {
            std::unique_ptr<EventQueue> event_queue;
            std::shared_ptr<Torrent> torrent;
        };
		tracker::Manager tracker_manager_; // Keep this before torrents_ so it is destroyed after
        std::vector<TorrentData> torrents_;
        std::mutex mutex_;
        EventCallbacks callbacks_;
    };

    Session::Session(EventCallbacks callbacks) : implementation_(std::make_unique<Implementation>(callbacks))
    {
    }

    Session::~Session() = default;

    TorrentHandle Session::AddTorrent(const TorrentParameters &params)
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