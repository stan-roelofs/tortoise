#include <tortoise/session.hpp>

namespace tortoise
{
    Session::Session(Parameters parameters) : running_(false)
    {
        if (parameters.start)
            Start();
    }

    Session::~Session()
    {
        Stop();
    }

    Torrent::Handle Session::AddTorrent(const Torrent::Parameters &params)
    {
        // if (params.save_path.empty())
        //  TODO get default path from settings

        auto torrent = std::make_shared<Torrent>(params);
        {
            std::lock_guard lock(mutex_);
            torrents_.push_back(torrent);
        }
        return Torrent::Handle(torrent);
    }

    void Session::RemoveTorrent(Torrent::Handle handle)
    {
        if (!handle.IsValid())
            return;

        std::lock_guard lock(mutex_);
        auto it = std::find_if(torrents_.begin(), torrents_.end(), [&](const std::shared_ptr<Torrent> &torrent)
                               { return Torrent::Handle(torrent) == handle; });
        if (it != torrents_.end()) // TODO can this block if we need to close connections etc? may need to implement this in a different way
            torrents_.erase(it);
    }

    void Session::ThreadFunc(Session &session)
    {
        session.Run();
    }

    void Session::Start()
    {
        std::lock_guard lock(mutex_);

        if (running_)
            return;

        running_ = true;
        thread_ = std::thread(ThreadFunc, std::ref(*this));
    }

    void Session::Stop()
    {
        std::lock_guard lock(mutex_);

        if (!running_)
            return;

        running_ = false;
        if (thread_.joinable())
            thread_.join();
    }

    void Session::Run()
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
}