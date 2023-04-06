#include "udp_tracker_connection.hpp"

#include <array>
#include <cassert>
#include <chrono>
#include <cstring>
#include <random>

#include <tortoise/exceptions.hpp>

#include "../log.hpp"

namespace tortoise
{
    // TODO this code is a mess, clean it up
    // TODO handle errors (action 3)

    namespace
    {
        constexpr std::uint64_t MAGIC_PROTOCOL_CONSTANT = 0x41727101980;
        constexpr int CONNECT_REQUEST_SIZE = 16;
        constexpr int CONNECT_RESPONSE_SIZE = 16;
        constexpr int ANNOUNCE_REQUEST_SIZE = 98;
        constexpr int ANNOUNCE_RESPONSE_MINIMUM_SIZE = 20;

        enum class Action : uint32_t
        {
            Connect = 0,
            Announce = 1,
            Scrape = 2,
            Error = 3
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

#pragma pack(push, 1)
        struct UDPConnectRequest
        {
            std::uint64_t protocol_constant;
            std::uint32_t action;
            std::uint32_t transaction_id;
        };

        struct UDPConnectResponse
        {
            std::uint32_t action;
            std::uint32_t transaction_id;
            std::uint64_t connection_id;
        };

        struct UDPAnnounceRequest
        {
            std::uint64_t connection_id;
            std::uint32_t action;
            std::uint32_t transaction_id;
            std::uint8_t info_hash[20];
            std::uint8_t peer_id[20];
            std::uint64_t downloaded;
            std::uint64_t left;
            std::uint64_t uploaded;
            std::uint32_t event;
            std::uint32_t ip_address;
            std::uint32_t key;
            std::uint32_t num_want;
            std::uint16_t port;
        };

        struct UDPAnnounceResponse
        {
            std::uint32_t action;
            std::uint32_t transaction_id;
            std::uint32_t interval;
            std::uint32_t leechers;
            std::uint32_t seeders;

            struct Peer
            {
                std::uint32_t ip_address;
                std::uint16_t port;
            };
        };
#pragma pack(pop)
        static_assert(sizeof(UDPConnectRequest) == CONNECT_REQUEST_SIZE);
        static_assert(sizeof(UDPConnectResponse) == CONNECT_RESPONSE_SIZE);
        static_assert(sizeof(UDPAnnounceRequest) == ANNOUNCE_REQUEST_SIZE);
        static_assert(sizeof(UDPAnnounceResponse) == ANNOUNCE_RESPONSE_MINIMUM_SIZE);

        UDPAnnounceRequest CreateAnnounceRequest(const AnnounceParameters &parameters,
                                                 std::uint32_t transaction_id, std::uint64_t connection_id)
        {
            UDPAnnounceRequest result;

            result.connection_id = Socket::ToNetworkByteOrder(connection_id);
            result.action = Socket::ToNetworkByteOrder(static_cast<std::uint32_t>(Action::Announce));
            result.transaction_id = Socket::ToNetworkByteOrder(transaction_id);
            std::memcpy(result.info_hash, parameters.info_hash.GetBytes().data(), 20);
            std::memcpy(result.peer_id, parameters.peer_id.Get().data(), 20);
            result.downloaded = Socket::ToNetworkByteOrder(parameters.downloaded);
            result.left = Socket::ToNetworkByteOrder(parameters.left);
            result.uploaded = Socket::ToNetworkByteOrder(parameters.uploaded);
            result.event = Socket::ToNetworkByteOrder(static_cast<std::uint32_t>(parameters.event));
            if (!parameters.ip || parameters.ip->IsIPv6()) // the IP address field in the request remains 32bits wide which makes this field not usable under IPv6.
            {
                result.ip_address = 0;
            }
            else
            {
                const auto ip_address = parameters.ip->ToVector();
                assert(ip_address.size() == 4);
                result.ip_address = Socket::ToNetworkByteOrder(*reinterpret_cast<const std::uint32_t *>(ip_address.data())); // TODO is this correct?
            }

            result.key = Socket::ToNetworkByteOrder(parameters.key ? *parameters.key : 0);
            result.num_want = Socket::ToNetworkByteOrder(parameters.numwant ? *parameters.numwant : -1);
            result.port = Socket::ToNetworkByteOrder(parameters.port);

            return result;
        }

        std::shared_ptr<AnnounceResponse> ParseResponse(const std::uint8_t *packet, const int packet_size, Socket::InternetProtocol protocol)
        {
            if (packet_size < ANNOUNCE_RESPONSE_MINIMUM_SIZE)
            {
                LOG("UDPTrackerConnection", "packet size is too small");
                return {};
            }

            int stride_size = 0;
            if (protocol == Socket::InternetProtocol::IPv4)
                stride_size = 6;
            else if (protocol == Socket::InternetProtocol::IPv6)
                stride_size = 18;
            else
            {
                LOG("UDPTrackerConnection", "unknown protocol");
                return {};
            }

            if ((packet_size - ANNOUNCE_RESPONSE_MINIMUM_SIZE) % stride_size != 0)
            {
                LOG("UDPTrackerConnection", "packet size is invalid: %d", packet_size);
                return {};
            }

            std::shared_ptr<AnnounceResponse> result = std::make_shared<AnnounceResponse>();

            result->interval = Socket::FromNetworkByteOrder(((std::uint32_t *)packet)[2]);
            result->incomplete = Socket::FromNetworkByteOrder(((std::uint32_t *)packet)[3]);
            result->complete = Socket::FromNetworkByteOrder(((std::uint32_t *)packet)[4]);

            const int nr_peers = (packet_size - ANNOUNCE_RESPONSE_MINIMUM_SIZE) / stride_size;
            for (int i = 0; i < nr_peers; i++)
            {
                const std::uint8_t *peer_start = packet + ANNOUNCE_RESPONSE_MINIMUM_SIZE + i * stride_size;
                if (stride_size == 6)
                {
                    const IPAddress ip_address(IPAddress::ipv4_address_t{peer_start[0], peer_start[1], peer_start[2], peer_start[3]});
                    const std::uint16_t port = Socket::FromNetworkByteOrder(((std::uint16_t *)peer_start)[2]);
                    result->peers.push_back(AnnounceResponse::PeerInfo{ip_address.ToString(), port});
                }
                else if (stride_size == 18)
                {
                    const IPAddress ip_address(IPAddress::ipv6_address_t{
                        peer_start[0], peer_start[1], peer_start[2], peer_start[3],
                        peer_start[4], peer_start[5], peer_start[6], peer_start[7],
                        peer_start[8], peer_start[9], peer_start[10], peer_start[11],
                        peer_start[12], peer_start[13], peer_start[14], peer_start[15]});
                    const std::uint16_t port = Socket::FromNetworkByteOrder(((std::uint16_t *)peer_start)[8]);
                    result->peers.push_back(AnnounceResponse::PeerInfo{ip_address.ToString(), port});
                }
            }

            return result;
        }
    }

    UDPTrackerConnection::UDPTrackerConnection(const URL &url) : TrackerConnection(url), timeout_(0u), socket_(Socket::TransportProtocol::UDP)
    {
        if (url.GetProtocol() != "udp")
            throw UnsupportedProtocolException(url.GetProtocol());
    }

    UDPTrackerConnection::~UDPTrackerConnection()
    {
        if (thread_.joinable())
            thread_.join();
    }

    void UDPTrackerConnection::ThreadFunc(UDPTrackerConnection *connection)
    {
        connection->Announce();
    }

    bool UDPTrackerConnection::Announce(const AnnounceParameters &parameters, std::function<void(Result, std::shared_ptr<AnnounceResponse> response)> result_callback, unsigned int timeout)
    {
        if (thread_.joinable())
            return false;

        timeout_ = timeout;
        result_callback_ = result_callback;
        parameters_ = std::make_unique<AnnounceParameters>(parameters);

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

        // TODO timeout
        while (true)
        {

            /* 1. Send connect request
             *    - Choose a random transaction ID.
             *    - Fill the connect request structure.
             *    - Send the packet.
             */
            std::random_device r;
            std::mt19937 gen(r());
            std::uniform_int_distribution<uint32_t> dist;

            std::uint32_t transaction_id = dist(gen);

            {
                std::uint8_t packet[CONNECT_REQUEST_SIZE];                                                              // 8 + 4 + 4
                ((std::uint64_t *)packet)[0] = Socket::ToNetworkByteOrder(MAGIC_PROTOCOL_CONSTANT);                     // Offset 0 : 64-bit magic constant
                ((std::uint32_t *)packet)[2] = Socket::ToNetworkByteOrder(static_cast<std::uint32_t>(Action::Connect)); // Offset 8 : 32-bit action
                ((std::uint32_t *)packet)[3] = Socket::ToNetworkByteOrder(transaction_id);                              // Offset 12 : 32-bit transaction ID

                if (!socket_.Send(packet, CONNECT_REQUEST_SIZE, timeout_))
                {
                    LOG("UDPTrackerConnection", "Failed to send connect request.");
                    result_callback_(Result::Failure, nullptr);
                    return;
                }
            }

            ConnectionId connection_id;

            /* 2. Receive connect response
             *    - Receive the response.
             *    - Check whether the packet is at least 16 bytes.
             *    - Check whether the transaction ID is equal to the one you chose.
             *    - Check whether the action is connect.
             *    - Store the connection ID for future use.
             */
            {
                std::uint8_t packet[CONNECT_RESPONSE_SIZE];
                if (!socket_.Receive(packet, CONNECT_RESPONSE_SIZE, timeout_))
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

                LOG("UDPTrackerConnection", "Server accepted connect request. Our connection id is: %" PRId64, connection_id.GetId());
            }

            /* 3. Send announce request
             *      - Choose a random transaction ID.
             *      - Fill the announce request structure.
             *      - Send the packet.
             */
            {
                transaction_id = dist(gen);
                const auto packet = CreateAnnounceRequest(*parameters_, transaction_id, connection_id.GetId());
                if (!socket_.Send(&packet, ANNOUNCE_REQUEST_SIZE, timeout_))
                {
                    LOG("UDPTrackerConnection", "Failed to send announce request.");
                    result_callback_(Result::Failure, nullptr);
                    return;
                }
            }

            /* 4. Receive announce response
             *      - Receive the response.
             *      - Check whether the packet is at least 20 bytes.
             *      - Check whether the transaction ID is equal to the one you chose.
             *      - Check whether the action is announce.
             *      - Check whether the interval is not zero.
             *      - Check whether the leechers and seeders are not negative.
             *      - Check whether the number of peers is a multiple of 6.
             *      - Check whether the number of peers is not greater than 50.
             *      - Store the peers.
             */
            {
                std::vector<std::uint8_t> result;
                constexpr std::size_t RECEIVE_BUFFER_SIZE = 0xFFFF;
                result.reserve(RECEIVE_BUFFER_SIZE);
                const int received = socket_.Receive(result.data(), RECEIVE_BUFFER_SIZE, timeout_);
                if (received == -1 || received < ANNOUNCE_RESPONSE_MINIMUM_SIZE)
                {
                    LOG("UDPTrackerConnection", "Failed to receive announce response.");
                    result_callback_(Result::Failure, nullptr);
                    return;
                }

                LOG("UDPTrackerConnection", "Server accepted announce request");

                const std::uint8_t *packet = result.data();
                const std::uint32_t action = Socket::FromNetworkByteOrder(((std::uint32_t *)packet)[0]);
                const std::uint32_t received_transaction_id = Socket::FromNetworkByteOrder(((std::uint32_t *)packet)[1]);

                if (received_transaction_id != transaction_id)
                {
                    LOG("UDPTrackerConnection", "Transaction ID mismatch.");
                    result_callback_(Result::Failure, nullptr);
                    return;
                }

                if (action != static_cast<std::uint32_t>(Action::Announce))
                {
                    LOG("UDPTrackerConnection", "Invalid action.");
                    result_callback_(Result::Failure, nullptr);
                    return;
                }

                std::shared_ptr<AnnounceResponse> response = ParseResponse(packet, received, socket_.GetInternetProtocol());
                if (!response)
                {
                    LOG("UDPTrackerConnection", "Failed to parse announce response.");
                    result_callback_(Result::Failure, nullptr);
                    return;
                }

                result_callback_(Result::Success, response);
                return;
            }
        }
    }
}