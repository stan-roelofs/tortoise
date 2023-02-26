#ifndef TORTOISE_TRACKER_HPP
#define TORTOISE_TRACKER_HPP

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "ip_address.hpp"
#include "sha1_hash.hpp"

namespace tortoise
{
    struct TrackerRequest
    {
        TrackerRequest(const SHA1Hash &info_hash, const std::array<std::uint8_t, 20> peer_id);

        //! \brief The info hash of the torrent.
        SHA1Hash info_hash;

        //! \brief The peer id of the client.
        std::array<std::uint8_t, 20> peer_id;

        //! \brief The port number that the client is listening on.
        std::uint16_t port;

        //! \brief The number of bytes downloaded so far.
        std::uint64_t downloaded;

        //! \brief The number of bytes uploaded so far.
        std::uint64_t uploaded;

        //! \brief The number of bytes left to download.
        std::uint64_t left;

        //! \brief Indicates whether the client supports compact responses (see BEP 23). 
		std::optional<bool> compact;

        //! \brief Indicates that the tracker can omit peer id fields in the peers dictionary. Ignored if compact is enabled.
        std::optional<bool> no_peer_id;

        enum class Event
        {
            //! \brief Sent when the client starts downloading the torrent.
            Started,

            //! \brief Sent when the client stops downloading the torrent.
            Stopped,

            //! \brief Sent when the client completes the download. Must not be sent if the download was already 100% when the client started.
            Completed
        };

        //! \brief The event that the client is reporting.
        std::optional<Event> event;

        //! \brief The true IP address of the client machine.
        std::optional<IPAddress> ip;

        //! \brief The number of peers that the client would like to receive from the tracker.
        std::optional<std::uint64_t> numwant;

        //! \brief A string that the client should send back on its next announcements.
        std::optional<std::string> tracker_id;
    };

    struct TrackerResponse
    {
        TrackerResponse();

        //! \brief (Optional) A human-readable error message as to why the request failed.
        std::string failure_reason;

        //! \brief (Optional) Similar to failure reason, but in this case the request was still processed normally.
        std::string warning_message;

        struct PeerInfo
        {
            std::string peer_id; // The 20-byte self-selected peer id of the peer. This may be empty.
            std::string ip;
            std::uint16_t port;
        };

        //! \brief Interval in seconds that the client should wait between sending regular requests to the tracker.
        std::uint64_t interval;

        //! \brief The minimum announce interval in seconds that the client should wait between sending requests to the tracker. 0 if unspecified.
        std::uint64_t min_interval;

        //! \brief A string that the client should send back on its next announcements.
        std::string tracker_id_;

        //! \brief The number of peers with the entire file, i.e. seeders.
        std::uint64_t complete_;

        //! \brief The number of non-seeder peers, aka "leechers".
        std::uint64_t incomplete_;

        //! \brief A list of peers.
        std::vector<PeerInfo> peers_;
    };
} // namespace tortoise

#endif