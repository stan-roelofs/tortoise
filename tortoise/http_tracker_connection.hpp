#ifndef TORTOISE_HTTP_TRACKER_CONNECTION_HPP
#define TORTOISE_HTTP_TRACKER_CONNECTION_HPP

#include <tortoise/tracker_connection.hpp>

namespace tortoise
{
    //! \brief Implements the legacy HTTP tracker protocol.
    class HTTPTrackerConnection : public TrackerConnection
    {
    public:
        /*! \brief Creates a new HTTP tracker connection.
         *  \param url The URL of the tracker.
         *  \throws UnsupportedProtocolException If the protocol is not supported.
         */
        HTTPTrackerConnection(const URL &url);

        /*! \brief Destroys the HTTP tracker connection. This may block
         */
        ~HTTPTrackerConnection() override;

        HTTPTrackerConnection(const HTTPTrackerConnection &) = delete;
        HTTPTrackerConnection &operator=(const HTTPTrackerConnection &) = delete;
        HTTPTrackerConnection(HTTPTrackerConnection &&) = delete;
        HTTPTrackerConnection &operator=(HTTPTrackerConnection &&) = delete;

        bool Announce(const AnnounceParameters &parameters, std::function<void(Result, std::shared_ptr<AnnounceResponse> response)> result_callback, unsigned int timeout) override;

    private:
        std::shared_ptr<AnnounceResponse> ParseResponse(const std::string &response_string);

        std::unique_ptr<http::AsyncRequest> request_;
    };
} // namespace tortoise

#endif // TORTOISE_HTTP_TRACKER_CONNECTION_HPP
