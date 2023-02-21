#include <tortoise/session.hpp>

namespace tortoise
{
    Session::Session()
    {
    }

    Session::~Session()
    {
    }

    Torrent::Handle Session::AddTorrent(const Torrent::Parameters &params)
    {
        auto torrent = std::make_shared<Torrent>(params);
        torrents_.push_back(torrent);
        return Torrent::Handle(torrent);
    }
}