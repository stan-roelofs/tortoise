#ifndef TORTOISE_EVENT_HPP
#define TORTOISE_EVENT_HPP

#include <functional>
#include <string>

#include <tortoise/peer_info.hpp>
#include <tortoise/torrent.hpp>

namespace tortoise
{


    namespace event
    {
        struct TorrentAdded
        {
            TorrentHandle handle;
        };

        struct PeerStatusChanged
        {
            TorrentHandle handle;
            PeerInfo info;
            PeerStatus status;
        };

        struct Callbacks
        {
            std::function<void(TorrentAdded)> torrent_added;
            std::function<void(PeerStatusChanged)> peer_status_changed;
        };
    }

} // namespace tortoise

#endif