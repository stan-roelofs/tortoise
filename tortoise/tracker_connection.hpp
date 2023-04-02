#ifndef TORTOISE_TRACKER_CONNECTION_HPP
#define TORTOISE_TRACKER_CONNECTION_HPP

#include <functional>
#include <memory>

#include "tracker_announce.hpp"

#include "http/request.hpp"

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
} // namespace tortoise

#endif // TORTOISE_TRACKER_CONNECTION_HPP