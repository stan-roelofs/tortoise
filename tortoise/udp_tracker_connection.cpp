#include "udp_tracker_connection.hpp"

#include <array>
#include <random>

#include "log.hpp"

namespace tortoise
{
    namespace
    {
        constexpr std::uint64_t MAGIC_PROTOCOL_CONSTANT = 0x41727101980;
        constexpr int CONNECT_REQUEST_SIZE = 16;
        constexpr int CONNECT_RESPONSE_SIZE = 16;

        enum class Action : uint32_t
        {
            Connect = 0,
            Announce = 1,
            Scrape = 2
        };
    }

    UDPTrackerConnection::UDPTrackerConnection(const URL &url) : TrackerConnection(url), timeout_(0u), socket_(Socket::TransportProtocol::UDP)
    {
        if (url.GetProtocol() != "udp")
            throw UnsupportedProtocolException(url.GetProtocol());
    }

    void UDPTrackerConnection::ThreadFunc(UDPTrackerConnection *connection)
    {
        connection->Announce();
    }

    bool UDPTrackerConnection::Announce(const AnnounceParameters &, std::function<void(Result, std::shared_ptr<AnnounceResponse> response)> result_callback, unsigned int timeout)
    {
        if (thread_.joinable())
            return false;

        timeout_ = timeout;
        result_callback_ = result_callback;

        thread_ = std::thread(ThreadFunc, this);
        return true;
    }

    void UDPTrackerConnection::Announce()
    {
        if (!socket_.Connect(url_.GetHost(), url_.GetPort(), timeout_))
        {
            LOG("UDPTrackerConnection", "Failed to connect to tracker.");
            result_callback_(Result::Failure, nullptr);
            return;
        }

        /* 1. Send connect request
         *    - Choose a random transaction ID.
         *    - Fill the connect request structure.
         *    - Send the packet.
         */
        std::random_device r;
        std::mt19937 gen(r());
        std::uniform_int_distribution<uint32_t> dist;

        const std::uint32_t transaction_id = dist(gen);

        {
            std::uint8_t packet[CONNECT_REQUEST_SIZE];                                                              // 8 + 4 + 4
            ((std::uint64_t *)packet)[0] = Socket::ToNetworkByteOrder(MAGIC_PROTOCOL_CONSTANT);                     // Offset 0 : 64-bit magic constant
            ((std::uint32_t *)packet)[2] = Socket::ToNetworkByteOrder(static_cast<std::uint32_t>(Action::Connect)); // Offset 8 : 32-bit action
            ((std::uint32_t *)packet)[3] = Socket::ToNetworkByteOrder(transaction_id);                              // Offset 12 : 32-bit transaction ID

            if (!socket_.SendAll(packet, CONNECT_REQUEST_SIZE, timeout_))
            {
                LOG("UDPTrackerConnection", "Failed to send connect request.");
                result_callback_(Result::Failure, nullptr);
                return;
            }
        }

        std::uint64_t connection_id = 0;

        /* 2. Receive connect response
         *    - Receive the response.
         *    - Check whether the packet is at least 16 bytes.
         *    - Check whether the transaction ID is equal to the one you chose.
         *    - Check whether the action is connect.
         *    - Store the connection ID for future use.
         */
        {
            std::uint8_t packet[CONNECT_RESPONSE_SIZE];
            if (!socket_.ReceiveAll(packet, CONNECT_RESPONSE_SIZE, timeout_))
            {
                LOG("UDPTrackerConnection", "Failed to receive connect response.");
                result_callback_(Result::Failure, nullptr);
                return;
            }

            const std::uint32_t action = Socket::FromNetworkByteOrder(((std::uint32_t *)packet)[0]);
            const std::uint32_t received_transaction_id = Socket::FromNetworkByteOrder(((std::uint32_t *)packet)[1]);
            connection_id = Socket::FromNetworkByteOrder(((std::uint64_t *)packet)[1]);

            if (received_transaction_id != transaction_id)
            {
                LOG("UDPTrackerConnection", "Transaction ID mismatch.");
                result_callback_(Result::Failure, nullptr);
                return;
            }

            if (action != static_cast<std::uint32_t>(Action::Connect))
            {
                LOG("UDPTrackerConnection", "Invalid action.");
                result_callback_(Result::Failure, nullptr);
                return;
            }

            LOG("UDPTrackerConnection", "Server accepted connect request. Our connection id is: %" PRId64, connection_id);
        }
    }
}