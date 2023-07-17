#ifndef TORTOISE_UDP_TRACKER_CONNECTION_HPP
#define TORTOISE_UDP_TRACKER_CONNECTION_HPP

#include <chrono>
#include <memory>

#include "tracker_connection.hpp"
#include "socket.hpp"

namespace tortoise
{
    //! \brief Implements the UDP tracker protocol as described in BEP 15.
    class UDPTrackerConnection : public TrackerConnection
    {
    public:
        UDPTrackerConnection();
        ~UDPTrackerConnection() override;

        bool Announce(const URL &url, const AnnounceParameters &parameters) override;
        bool Process() override;
        Result GetLastResult() const override;
        void Cancel() override;

    private:
        enum class ReceiveResult
        {
            Finished,
            Unfinished,
            Timeout,
            Error
        };

        void CreateConnectRequest();
        void CreateAnnounceRequest();
        bool Send();
        ReceiveResult Receive();
        void ResizeBuffer(std::size_t size);

        enum class State
        {
            Idle,
            SendConnectRequest,
            ReceiveConnectResponse,
            SendAnnounceRequest,
            ReceiveAnnounceResponse
        };

        /*! \brief A connection ID is a 64-bit number that is used to identify a connection to a tracker. It is sent by the tracker in response to a connect request.
         *         A client can use a connection ID until one minute after it has received it.
         */
        class ConnectionId
        {
        public:
            ConnectionId() : id(0) {}
            ConnectionId(std::uint64_t id) : id(id), receive_time(std::chrono::steady_clock::now()) {}

            std::uint64_t GetId() const { return id; }
            bool IsValid() const
            {
                const auto now = std::chrono::steady_clock::now();
                const auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - receive_time);
                return duration.count() < 60;
            }

        private:
            std::uint64_t id;
            std::chrono::steady_clock::time_point receive_time;
        };

        Result result_;
        std::unique_ptr<AnnounceParameters> parameters_;
        State state_;
        Socket socket_;
        ConnectionId connection_id_;
        std::uint32_t transaction_id_;
        std::vector<std::uint8_t> buffer_;
        std::size_t current_buffer_position_;
        std::chrono::steady_clock::time_point timeout_time_;
        unsigned nr_timeouts_;
    };

} // namespace tortoise

#endif // TORTOISE_UDP_TRACKER_CONNECTION_HPP