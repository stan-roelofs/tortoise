#include <tortoise/session.hpp>

#include "event_queue.hpp"
#include "torrent_impl.hpp"
#include "tracker/tracker_manager.hpp"

namespace
{

}

namespace tortoise
{
    class Session::Implementation
    {
    public:
        Implementation(EventCallbacks callbacks) : event_queue_(callbacks)
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

            auto torrent = std::make_shared<Torrent>(parameters, tracker_manager_, event_queue_);
            {
                std::lock_guard lock(mutex_);
                torrents_.push_back(torrent);
            }

            TorrentHandle handle(torrent);
            if (event_queue_.EventEnabled(EventType::TorrentAdded))
                event_queue_.PushEvent(std::make_unique<TorrentAddedEvent>(handle));

            return handle;
        }

        void RemoveTorrent(TorrentHandle handle)
        {
            if (!handle.IsValid())
                return;

            std::lock_guard lock(mutex_);
            auto it = std::find_if(torrents_.begin(), torrents_.end(), [&](const std::shared_ptr<Torrent> &torrent)
                                   { return TorrentHandle(torrent) == handle; });
            if (it != torrents_.end()) // TODO can this block if we need to close connections etc? may need to implement this in a different way
                torrents_.erase(it);
        }

        void HandleEvents()
        {
            event_queue_.HandleEvents();
        }

    private:
        std::vector<std::shared_ptr<Torrent>> torrents_;
        tracker::Manager tracker_manager_;
        std::mutex mutex_;
        EventQueue event_queue_;
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