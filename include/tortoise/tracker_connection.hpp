#ifndef TORTOISE_TRACKER_CONNECTION_HPP
#define TORTOISE_TRACKER_CONNECTION_HPP

#include <functional>
#include <memory>

#include "http/request.hpp"
#include "tracker_announce.hpp"

namespace tortoise
{
    class TrackerConnection
    {
    public:
        enum class Result
        {
            Success,
            Failure
        };

        TrackerConnection(const URL &url);
        virtual ~TrackerConnection();

        /**
         * \brief Asynchronously sends an announce request to the tracker.
         * \param parameters The parameters to send to the tracker.
         * \param result_callback The callback to call when the response is received.
         * \param timeout The timeout for each socket operation in milliseconds.
         * \returns True if the request was sent successfully.
         */
        virtual bool Announce(const AnnounceParameters &parameters, std::function<void(Result, std::shared_ptr<AnnounceResponse> response)> result_callback, unsigned int timeout) = 0;

    protected:
        const URL url_;
    };

    class TrackerConnectionFactory
    {
    public:
        /*! \brief Creates a tracker connection for the specified URL.
         *  \param url The URL of the tracker.
         *  \returns A tracker connection object.
         *  \throws UnsupportedProtocolException If the protocol is not supported.
         */
        static std::unique_ptr<TrackerConnection> Create(const URL &url);
    };
} // namespace tortoise

#endif // TORTOISE_TRACKER_CONNECTION_HPP