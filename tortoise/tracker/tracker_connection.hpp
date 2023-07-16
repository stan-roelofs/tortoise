#ifndef TORTOISE_TRACKER_CONNECTION_HPP
#define TORTOISE_TRACKER_CONNECTION_HPP

#include <functional>
#include <memory>

#include "tracker_announce.hpp"

namespace tortoise
{
    class TrackerConnection
    {
    public:
        struct Result
        {
            bool success;
            std::optional<AnnounceResponse> response;
        };

        TrackerConnection();

        TrackerConnection(const TrackerConnection &) = delete;
        TrackerConnection &operator=(const TrackerConnection &) = delete;
        TrackerConnection(TrackerConnection &&) = delete;
        TrackerConnection &operator=(TrackerConnection &&) = delete;

        virtual ~TrackerConnection();

        /*! \brief Starts an announce request.
         *  \param url The URL of the tracker.
         *  \param parameters The parameters to send to the tracker.
         *  \returns True if successful. False if a request is already pending.
         */
        virtual bool Announce(const URL &url, const AnnounceParameters &parameters) = 0;

        /*! \brief Processes the current request without blocking. This should be called repeatedly until it returns true.
         *  \returns True if the request is complete.
         */
        virtual bool Process() = 0;

        //! \brief Returns the result of the last request.
        virtual Result GetLastResult() const = 0;

        //! \brief Cancels the current request.
        virtual void Cancel() = 0;
    };
} // namespace tortoise

#endif // TORTOISE_TRACKER_CONNECTION_HPP