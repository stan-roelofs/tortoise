#ifndef TORTOISE_ANNOUNCE_PARAMETERS_HPP
#define TORTOISE_ANNOUNCE_PARAMETERS_HPP

#include <optional>
#include <vector>

#include <tortoise/peer_info.hpp>

#include "../network/ip_address.hpp"
#include "../network/url.hpp"

namespace tortoise
{
    namespace tracker
    {
        struct AnnounceParameters
        {
            AnnounceParameters(const std::array<uint8_t, 20> &info_hash, const PeerId &peer_id);

            //! \brief The info hash of the torrent.
            const std::array<uint8_t, 20> info_hash;

            //! \brief The peer id of the client.
            const PeerId peer_id;

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

            //! \details These values are specified in BEP 15
            enum class Event
            {
                None = 0,

                //! \brief Sent when the client completes the download. Must not be sent if the download was already 100% when the client started.
                Completed = 1,

                //! \brief Sent when the client starts downloading the torrent.
                Started = 2,

                //! \brief Sent when the client stops downloading the torrent.
                Stopped = 3
            };

            //! \brief The event that the client is reporting.
            Event event;

            //! \brief The true IP address of the client machine.
            std::optional<network::IPAddress> ip;

            //! \brief The number of peers that the client would like to receive from the tracker.
            std::optional<std::uint32_t> numwant;

            //! \brief An additional identification that is not shared with any other peers. It is intended to allow a client to prove their identity should their IP address change.
            std::optional<std::uint32_t> key;

            //! \brief A string that the client should send back on its next announcements.
            std::optional<std::string> tracker_id;
        };

        struct AnnounceResponse
        {
            AnnounceResponse();

            //! \brief A human-readable error message as to why the request failed.
            std::optional<std::string> failure_reason;

            //! \brief Similar to failure reason, but in this case the request was still processed normally.
            std::optional<std::string> warning_message;

            //! \brief Interval in seconds that the client should wait between sending regular requests to the tracker.
            std::uint64_t interval;

            //! \brief The minimum announce interval in seconds that the client should wait between sending requests to the tracker. 0 if unspecified.
            std::optional<std::uint64_t> min_interval;

            //! \brief A string that the client should send back on its next announcements. If not specified, the client should use the previous value.
            std::optional<std::string> tracker_id;

            //! \brief The number of peers with the entire file, i.e. seeders.
            std::uint64_t complete;

            //! \brief The number of non-seeder peers, aka "leechers".
            std::uint64_t incomplete;

            //! \brief A list of peers.
            std::vector<PeerInfo> peers;
        };
    } // namespace tracker
} // namespace tortoise

#endif // TORTOISE_ANNOUNCE_PARAMETERS_HPP