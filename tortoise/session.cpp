#include <tortoise/session.hpp>

#include "event_queue.hpp"
#include "torrent_impl.hpp"

namespace tortoise
{
    class Session::Implementation
    {
    public:
        Implementation(Parameters &parameters) : running_(false), event_queue_(parameters.event_mask)
        {
            if (parameters.start)
                Start();
        }

        ~Implementation()
        {
            Stop();
        }

        TorrentHandle AddTorrent(const TorrentParameters &parameters)
        {
            // if (params.save_path.empty())
            //  TODO get default path from settings

            // TODO check if torrent already exists

            auto torrent = std::make_shared<Torrent>(parameters, event_queue_);
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

        void Start()
        {
            std::lock_guard lock(mutex_);

            if (running_)
                return;

            running_ = true;
            thread_ = std::thread(ThreadFunc, std::ref(*this));
        }

        void Stop()
        {
            std::lock_guard lock(mutex_);

            if (!running_)
                return;

            running_ = false;
            if (thread_.joinable())
                thread_.join();
        }

        void PopEvents(const EventCallbacks &callbacks)
        {
            auto events = event_queue_.PopEvents();

            for (auto &event : events)
            {
                Event *e = event.get();

                // Note: we could use a visitor pattern here, but it is a lot of effort with little benefit.
                if (auto *added_event = dynamic_cast<TorrentAddedEvent *>(e))
                {
                    if (callbacks.torrent_added)
                        callbacks.torrent_added(added_event->torrent);
                }
                else if (auto *peer_status_event = dynamic_cast<PeerStatusChangedEvent *>(e))
                {
                    if (callbacks.peer_status_changed)
                        callbacks.peer_status_changed(peer_status_event->torrent, peer_status_event->ip, peer_status_event->port, peer_status_event->status);
                }
            }
        }

    private:
        static void ThreadFunc(Implementation &session)
        {
            session.Run();
        }
        void Run()
        {
            while (running_)
            {
                {
                    std::lock_guard lock(mutex_);
                    for (auto &torrent : torrents_)
                        torrent->Process();
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        bool running_ = true;
        std::vector<std::shared_ptr<Torrent>> torrents_;
        std::thread thread_;
        std::mutex mutex_;
        EventQueue event_queue_;
    };

    Session::Session(Parameters parameters) : implementation_(std::make_unique<Implementation>(parameters))
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

    void Session::Start()
    {
        implementation_->Start();
    }

    void Session::Stop()
    {
        implementation_->Stop();
    }

    void Session::PopEvents(const EventCallbacks &callbacks)
    {
        implementation_->PopEvents(callbacks);
    }
}