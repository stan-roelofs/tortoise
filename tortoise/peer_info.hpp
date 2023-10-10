#ifndef TORTOISE_PEER_INFO_HPP
#define TORTOISE_PEER_INFO_HPP

#include <cstdint>
#include <optional>
#include <string>

#include "peer_id.hpp"

namespace tortoise
{
    struct PeerInfo
    {
        PeerInfo(const std::string &ip, std::uint16_t port) : ip(ip), port(port) {}
        std::optional<PeerId> peer_id; // The 20-byte self-selected peer id of the peer.
        std::string ip;                // The peer's IP address either IPv6 (hexed) or IPv4 (dotted quad) or DNS name (string).
        std::uint16_t port;

        bool operator==(const PeerInfo &other) const
        {
            return ip == other.ip && port == other.port;
        }

        bool operator!=(const PeerInfo &other) const
        {
            return !(*this == other);
        }
    };
}

#endif