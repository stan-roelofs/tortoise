#ifndef TORTOISE_TRACKER_HPP
#define TORTOISE_TRACKER_HPP

#include <array>
#include <string>
#include <vector>

#include "socket.hpp"

namespace tortoise
{
    class Tracker
    {
    public:
        struct RequestParameters
        {
            RequestParameters();

            //! \brief The info hash of the torrent.
            std::array<std::uint8_t, 20> info_hash;

            //! \brief The peer id of the client.
            std::array<std::uint8_t, 20> peer_id;

            //! \brief The number of bytes downloaded so far.
            std::uint64_t downloaded;

            //! \brief The number of bytes uploaded so far.
            std::uint64_t uploaded;

            //! \brief The number of bytes left to download.
            std::uint64_t left;

            enum class Event
            {
                //! \brief The default event. Should be sent if no other event applies.
                None,

                //! \brief Sent when the client starts downloading the torrent.
                Started,

                //! \brief Sent when the client stops downloading the torrent.
                Stopped,

                //! \brief Sent when the client completes the download. Must not be sent if the download was already 100% when the client started.
                Completed
            };

            //! \brief (Optional) The event that the client is reporting.
            Event event;

            //! \brief (Optional) The number of peers that the client would like to receive from the tracker.
            std::uint64_t numwant;

            //! \brief (Optional) The port that the client is listening on.
            uint16_t port;

            //! \brief (Optional) A string that the client should send back on its next announcements.
            std::string tracker_id;
        };

        Tracker(const std::string &announce_url);
        ~Tracker();

        /*! \brief Sends an announce to the tracker and blocks until a response is received.
         *  \param request_parameters The parameters to use when sending the announce.
         *  \returns Whether communication with the tracker was successful.
         */
        bool Announce(const RequestParameters &request_parameters);

        struct PeerInfo
        {
            std::string peer_id;
            std::string ip;
            std::uint16_t port;
        };

        //! \returns The interval in seconds that the client should wait between sending requests to the tracker.
        std::uint64_t GetInterval() const;

        //! \returns The minimum announce interval in seconds that the client should wait between sending requests to the tracker. 0 if unspecified.
        std::uint64_t GetMinimumInterval() const;

        //! \returns The number of peers with the entire file, i.e. seeders.
        std::uint64_t GetComplete() const;

        //! \returns The number of non-seeder peers, aka "leechers".
        std::uint64_t GetIncomplete() const;

        //! \returns A list of peers.
        const std::vector<PeerInfo> &GetPeers() const;

    private:
        bool HandleResponse(const std::string &response);

        Socket socket_;

        //! \brief The announce URL of the tracker.
        std::string announce_url_;

        //! \brief Interval in seconds that the client should wait between sending regular requests to the tracker.
        std::uint64_t interval_;

        //! \brief The minimum announce interval in seconds that the client should wait between sending requests to the tracker. 0 if unspecified.
        std::uint64_t min_interval_;

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