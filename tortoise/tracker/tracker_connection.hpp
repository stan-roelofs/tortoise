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

        TrackerConnection(const URL &url);
        virtual ~TrackerConnection();

        /*! \brief Starts an announce request.
         *  \param parameters The parameters to send to the tracker.
         *  \returns True if successful. False if a request is already pending.
         */
        virtual bool Announce(const AnnounceParameters &parameters) = 0;

        /*! \brief Processes the current request without blocking. This should be called repeatedly until it returns true.
         *  \returns True if the request is complete.
         */
        virtual bool Process() = 0;

        //! \brief Returns the result of the last request.
        virtual Result GetLastResult() const = 0;

        //! \brief Cancels the current request.
        virtual void Cancel() = 0;

    protected:
        const URL url_;
    };
} // namespace tortoise

#endif // TORTOISE_TRACKER_CONNECTION_HPP