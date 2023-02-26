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
        auto torrent = std::make_shared<Torrent>(params);
        torrents_.push_back(torrent);
        return Torrent::Handle(torrent);
    }

    void Session::ThreadFunc(Session &session)
    {
        session.Run();
    }

    void Session::Start()
    {
        if (running_)
            return;

        running_ = true;
        thread_ = std::thread(ThreadFunc, std::ref(*this));
    }

    void Session::Stop()
    {
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
			for (const auto& torrent : torrents_)
				torrent->Process();
        }
    }
}