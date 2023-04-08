#ifndef TORTOISE_HTTP_TRACKER_CONNECTION_HPP
#define TORTOISE_HTTP_TRACKER_CONNECTION_HPP

#include "tracker_connection.hpp"
#include "../socket.hpp"

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

        bool Announce(const AnnounceParameters &parameters) override;
        bool Process() override;
        Result GetLastResult() const override;
        void Cancel() override;

    private:
        enum class State
        {
            Idle,
            Connect,
            SendRequest,
            ReceiveResponse
        };

        Socket socket_;
        Result result_;
        State state_;
        bool verified_;
        std::vector<std::uint8_t> buffer_;
        std::size_t current_buffer_position_;
    };
} // namespace tortoise

#endif // TORTOISE_HTTP_TRACKER_CONNECTION_HPP
