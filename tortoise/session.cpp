#include <tortoise/session.hpp>

#include "torrent_impl.hpp"

namespace tortoise
{
    class Session::Implementation
    {
    public:
        Implementation(Parameters &parameters) : running_(false)
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

            auto torrent = std::make_shared<Torrent>(parameters);
            {
                std::lock_guard lock(mutex_);
                torrents_.push_back(torrent);
            }
            return TorrentHandle(torrent);
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
}