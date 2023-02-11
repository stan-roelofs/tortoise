#ifndef TORTOISE_TRACKER_HPP
#define TORTOISE_TRACKER_HPP

#include <string>

#include "metainfo.hpp"
#include "socket.hpp"

namespace tortoise
{
    class Tracker
    {
    public:
        Tracker(const Metainfo &info);

        /*! \brief Sends an announce to the tracker and blocks until a response is received.
         *   \returns Whether communication with the tracker was successful.
         */
        bool Announce();

        struct PeerInfo
        {
            std::string peer_id;
            std::string ip;
            std::uint16_t port;
        };

        //! \returns The interval in seconds that the client should wait between sending requests to the tracker.
        uint64_t GetInterval() const;

        //! \returns The minimum announce interval in seconds that the client should wait between sending requests to the tracker. 0 if unspecified.
        uint64_t GetMinimumInterval() const;

        //! \returns The number of peers with the entire file, i.e. seeders.
        uint64_t GetComplete() const;

        //! \returns The number of non-seeder peers, aka "leechers".
        uint64_t GetIncomplete() const;

        //! \returns A list of peers.
        const std::vector<PeerInfo>& GetPeers() const;

    private:
        bool HandleResponse(const std::string &response);

        Metainfo info_;
        Socket socket_;

        uint64_t interval_;

        //! \brief The minimum announce interval in seconds that the client should wait between sending requests to the tracker. 0 if unspecified.
        uint64_t min_interval_;

        //! \brief A string that the client should send back on its next announcements.
        std::string tracker_id_;

        //! \brief The number of peers with the entire file, i.e. seeders.
        uint64_t complete_;

        //! \brief The number of non-seeder peers, aka "leechers".
        uint64_t incomplete_;

        //! \brief A list of peers.
        std::vector<PeerInfo> peers_;
    };
} // namespace tortoise

#endif