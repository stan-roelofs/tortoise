#ifndef TORTOISE_EVENT_HPP
#define TORTOISE_EVENT_HPP

#include <bitset>
#include <cstdint>
#include <functional>
#include <string>

#include <tortoise/torrent.hpp>

namespace tortoise
{
    enum class PeerStatus
    {
        Connecting,
        Connected,
        Disconnected,
    };

    struct EventCallbacks
    {
        std::function<void(TorrentHandle torrent)> torrent_added;
        std::function<void(TorrentHandle torrent, const std::string &ip, uint16_t port, PeerStatus status)> peer_status_changed;
    };

} // namespace tortoise

#endif