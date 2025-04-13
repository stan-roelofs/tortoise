#ifndef TORTOISE_TORRENT_PARAMETERS_HPP
#define TORTOISE_TORRENT_PARAMETERS_HPP

#include <chrono>

#include "metainfo.hpp"

namespace tortoise
{
    struct TorrentParameters
    {
        TorrentParameters(const Metainfo &info) : metainfo(info) {}
        Metainfo metainfo;
        std::filesystem::path save_path;

        unsigned max_peers = 30; // Maximum number of peers to connect to

        struct
        {
            unsigned request_queue_size = 20; // Number of requests to send to the peer at once

            struct
            {
                std::chrono::seconds keep_alive = std::chrono::seconds(120); // Keep alive timeout in seconds
                std::chrono::seconds connect = std::chrono::seconds(10);     // Connect timeout in seconds
                std::chrono::seconds handshake = std::chrono::seconds(10);   // Handshake timeout in seconds
            } timeouts;
        } peer;
    };
} // namespace tortoise

#endif