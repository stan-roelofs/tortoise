#include "connection.hpp"

#include <array>
#include <cassert>
#include <chrono>
#include <cstring>
#include <random>

#include <tortoise/exception.hpp>

#include "../network/socket.hpp"
#include "../util/log.hpp"
#include "../util/util.hpp"

namespace tortoise
{
    // TODO handle errors (action 3)
    // TODO this code is a complete mess

    namespace
    {
        const std::string_view log_tag = "Connection";

        constexpr std::uint64_t MAGIC_PROTOCOL_CONSTANT = 0x41727101980;
        constexpr int CONNECT_REQUEST_SIZE = 16;
        constexpr int CONNECT_RESPONSE_SIZE = 16;
        constexpr int ANNOUNCE_REQUEST_SIZE = 100;
        constexpr int ANNOUNCE_RESPONSE_MINIMUM_SIZE = 20;
        constexpr unsigned MAX_TIMEOUTS = 8;

        enum class Action : uint32_t
        {
            Connect = 0,
            Announce = 1,
            Scrape = 2,
            Error = 3
        };

        void CreateUDPAnnounceRequest(std::vector<std::uint8_t> &buffer, const tracker::AnnounceParameters &parameters,
                                      std::uint32_t transaction_id, std::uint64_t connection_id)
        {
            buffer.resize(ANNOUNCE_REQUEST_SIZE);
            std::uint8_t *packet = buffer.data();
            *(std::uint64_t *)(packet + 0) = network::HostToNetwork(connection_id);
            *(std::uint32_t *)(packet + 8) = network::HostToNetwork(static_cast<std::uint32_t>(Action::Announce));
            *(std::uint32_t *)(packet + 12) = network::HostToNetwork(transaction_id);
            std::memcpy(packet + 16, parameters.info_hash.data(), 20);
            std::memcpy(packet + 36, parameters.peer_id.Get().data(), 20);
            *(std::uint64_t *)(packet + 56) = network::HostToNetwork(parameters.downloaded);
            *(std::uint64_t *)(packet + 64) = network::HostToNetwork(parameters.left);
            *(std::uint64_t *)(packet + 72) = network::HostToNetwork(parameters.uploaded);
            *(std::uint32_t *)(packet + 80) = network::HostToNetwork(static_cast<std::uint32_t>(parameters.event));
            if (!parameters.ip || parameters.ip->IsIPv6()) // the IP address field in the request remains 32bits wide which makes this field not usable under IPv6.
            {
                *(std::uint32_t *)(packet + 84) = 0;
            }
            else
            {
                const auto ip_address = parameters.ip->ToVector();
                assert(ip_address.size() == 4);
                *(std::uint32_t *)(packet + 84) = network::HostToNetwork(*(std::uint32_t *)ip_address.data());
            }
            *(std::uint32_t *)(packet + 88) = parameters.key ? network::HostToNetwork(parameters.key.value()) : 0;
            *(std::uint32_t *)(packet + 92) = parameters.numwant ? network::HostToNetwork(parameters.numwant.value()) : 0;
            *(std::uint16_t *)(packet + 96) = network::HostToNetwork(parameters.port);
            *(std::uint16_t *)(packet + 98) = network::HostToNetwork((uint16_t)0); // extensions
        }

        std::optional<tracker::AnnounceResponse> ParseResponse(const std::uint8_t *packet, const int packet_size, network::InternetProtocol protocol)
        {
            if (packet_size < ANNOUNCE_RESPONSE_MINIMUM_SIZE)
            {
                LOG_ERROR(log_tag, "packet size is too small");
                return {};
            }

            int stride_size = 0;
            if (protocol == network::InternetProtocol::IPv4)
                stride_size = 6;
            else if (protocol == network::InternetProtocol::IPv6)
                stride_size = 18;
            else
            {
                LOG_ERROR(log_tag, "unknown protocol");
                return {};
            }

            if ((packet_size - ANNOUNCE_RESPONSE_MINIMUM_SIZE) % stride_size != 0)
            {
                LOG_ERROR(log_tag, std::format("packet size is invalid: {}", packet_size));
                return {};
            }

            tracker::AnnounceResponse result;

            result.interval = network::HostToNetwork(((std::uint32_t *)packet)[2]);
            result.incomplete = network::HostToNetwork(((std::uint32_t *)packet)[3]);
            result.complete = network::HostToNetwork(((std::uint32_t *)packet)[4]);

            const int nr_peers = (packet_size - ANNOUNCE_RESPONSE_MINIMUM_SIZE) / stride_size;
            for (int i = 0; i < nr_peers; i++)
            {
                const std::uint8_t *peer_start = packet + ANNOUNCE_RESPONSE_MINIMUM_SIZE + i * stride_size;
                if (stride_size == 6)
                {
                    const network::IPAddress ip_address(network::IPAddress::ipv4_address_t{peer_start[0], peer_start[1], peer_start[2], peer_start[3]});
                    const std::uint16_t port = network::HostToNetwork(((std::uint16_t *)peer_start)[2]);
                    result.peers.emplace_back(PeerInfo{ip_address.ToString(), port});
                }
                else if (stride_size == 18)
                {
                    const network::IPAddress ip_address(network::IPAddress::ipv6_address_t{
                        peer_start[0], peer_start[1], peer_start[2], peer_start[3],
                        peer_start[4], peer_start[5], peer_start[6], peer_start[7],
                        peer_start[8], peer_start[9], peer_start[10], peer_start[11],
                        peer_start[12], peer_start[13], peer_start[14], peer_start[15]});
                    const std::uint16_t port = network::HostToNetwork(((std::uint16_t *)peer_start)[8]);
                    result.peers.emplace_back(PeerInfo{ip_address.ToString(), port});
                }
            }

            return result;
        }

        std::uint32_t GenerateTransactionId()
        {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<std::uint32_t> dis;
            return dis(gen);
        }

        /* If a response is not received after 15 * 2 ^ n seconds, the client should retransmit the request,
         *  where n starts at 0 and is increased up to 8 (3840 seconds) after every retransmission
         */
        std::chrono::steady_clock::time_point GetTimeoutTime(unsigned int nr_timeouts)
        {
            if (nr_timeouts > 8)
                nr_timeouts = 8;
            const auto timeout = std::chrono::seconds(static_cast<int>(15 * std::pow(2, nr_timeouts)));
            return std::chrono::steady_clock::now() + timeout;
        }
    }

    class UDPTrackerConnection
    {
    public:
        UDPTrackerConnection();
        ~UDPTrackerConnection();

        std::optional<tracker::AnnounceResponse> Announce(const network::URL &url, const tracker::AnnounceParameters &parameters, std::shared_ptr<std::atomic_bool> cancel);

    private:
        enum class SendResult
        {
            Finished,
            Unfinished,
            Error
        };

        enum class ReceiveResult
        {
            Finished,
            Unfinished,
            Timeout,
            Error
        };

        void CreateConnectRequest();
        void CreateAnnounceRequest();
        SendResult Send();
        ReceiveResult Receive();
        void ResizeBuffer(std::size_t size);

        enum class State
        {
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

        std::shared_ptr<std::atomic_bool> cancel_;
        std::unique_ptr<tracker::AnnounceParameters> parameters_;
        State state_;
        network::Socket socket_;
        ConnectionId connection_id_;
        std::uint32_t transaction_id_;
        ByteVector buffer_;
        std::size_t current_buffer_position_;
        std::chrono::steady_clock::time_point timeout_time_;
        unsigned nr_timeouts_;
    };

    UDPTrackerConnection::UDPTrackerConnection() : state_(State::SendConnectRequest),
                                                   socket_(network::TransportProtocol::UDP),
                                                   transaction_id_(0),
                                                   current_buffer_position_(0u),
                                                   nr_timeouts_(0u)
    {
    }

    UDPTrackerConnection::~UDPTrackerConnection()
    {
        // TODO send a "stopped" event
    }

    std::optional<tracker::AnnounceResponse> UDPTrackerConnection::Announce(const network::URL &url, const tracker::AnnounceParameters &parameters, std::shared_ptr<std::atomic_bool> cancel)
    {
        cancel_ = cancel;
        parameters_ = std::make_unique<tracker::AnnounceParameters>(parameters);
        if (!socket_.Connect(url.GetHost(), url.GetPort()))
        {
            LOG_INFO(log_tag, std::format("failed to connect to {}:{}", url.GetHost(), url.GetPort()));
            return {};
        }

        LOG_INFO(log_tag, std::format("Connected to {}:{}", url.GetHost(), url.GetPort()));

        CreateConnectRequest();
        state_ = State::SendConnectRequest;

        while (!*cancel)
        {
            // TODO handle timeouts
            switch (state_)
            {
            case State::SendConnectRequest:
            {
                switch (Send())
                {
                case SendResult::Finished:
                    LOG_INFO(log_tag, "sent connect request");
                    ResizeBuffer(CONNECT_RESPONSE_SIZE);
                    state_ = State::ReceiveConnectResponse;
                    break;
                case SendResult::Unfinished:
                    break;
                case SendResult::Error:
                    LOG_ERROR(log_tag, "failed to send connect request");
                    return {};
                }
                break;
            }
            case State::ReceiveConnectResponse:
            {
                switch (Receive())
                {
                case ReceiveResult::Finished:
                {
                    const std::uint8_t *packet = buffer_.data();
                    const std::uint32_t action = network::HostToNetwork(((std::uint32_t *)packet)[0]);
                    const std::uint32_t received_transaction_id = network::HostToNetwork(((std::uint32_t *)packet)[1]);
                    std::uint64_t connection_id = network::HostToNetwork(((std::uint64_t *)packet)[1]);

                    if (received_transaction_id != transaction_id_)
                    {
                        LOG_ERROR(log_tag, "Transaction ID mismatch.");
                        return {};
                    }

                    if (action != static_cast<std::uint32_t>(Action::Connect))
                    {
                        LOG_ERROR(log_tag, "Invalid action.");
                        return {};
                    }

                    connection_id_ = ConnectionId(connection_id);
                    LOG_INFO(log_tag, std::format("Server accepted connect request. Our connection id is: {}", connection_id_.GetId()));

                    CreateAnnounceRequest();
                    state_ = State::SendAnnounceRequest;
                    break;
                }
                case ReceiveResult::Unfinished:
                    break;
                case ReceiveResult::Error:
                    LOG_ERROR(log_tag, "failed to receive connect response");
                    return {};
                case ReceiveResult::Timeout:
                    CreateConnectRequest();
                    state_ = State::SendConnectRequest;
                    break;
                }
                break;
            }
            case State::SendAnnounceRequest:
            {
                if (!connection_id_.IsValid())
                {
                    CreateConnectRequest();
                    state_ = State::SendConnectRequest;
                    break;
                }

                switch (Send())
                {
                case SendResult::Finished:
                    LOG_INFO(log_tag, "sent announce request");
                    ResizeBuffer(0xFFFF);
                    state_ = State::ReceiveAnnounceResponse;
                    break;
                case SendResult::Unfinished:
                    break;
                case SendResult::Error:
                    LOG_ERROR(log_tag, "failed to send announce request");
                    return {};
                }

                break;
            }
            case State::ReceiveAnnounceResponse:
            {
                switch (Receive())
                {
                case ReceiveResult::Finished:
                {
                    const std::uint8_t *packet = buffer_.data();
                    const std::uint32_t action = network::HostToNetwork(((std::uint32_t *)packet)[0]);
                    const std::uint32_t received_transaction_id = network::HostToNetwork(((std::uint32_t *)packet)[1]);

                    if (received_transaction_id != transaction_id_)
                    {
                        LOG_ERROR(log_tag, "Transaction ID mismatch.");
                        return {};
                    }

                    if (action != static_cast<std::uint32_t>(Action::Announce))
                    {
                        LOG_ERROR(log_tag, "Invalid action.");
                        return {};
                    }

                    return ParseResponse(packet, (int)current_buffer_position_, socket_.GetInternetProtocol());
                }
                case ReceiveResult::Unfinished:
                    break;
                case ReceiveResult::Error:
                    LOG_ERROR(log_tag, "failed to receive announce response");
                    return {};
                case ReceiveResult::Timeout:
                    CreateAnnounceRequest();
                    state_ = State::SendAnnounceRequest;
                    break;
                }
            }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        return {};
    }
    void UDPTrackerConnection::CreateConnectRequest()
    {
        transaction_id_ = GenerateTransactionId();
        ResizeBuffer(CONNECT_REQUEST_SIZE);

        std::uint8_t *packet = buffer_.data();
        ((std::uint64_t *)packet)[0] = network::HostToNetwork(MAGIC_PROTOCOL_CONSTANT);                     // Offset 0 : 64-bit magic constant
        ((std::uint32_t *)packet)[2] = network::HostToNetwork(static_cast<std::uint32_t>(Action::Connect)); // Offset 8 : 32-bit action
        ((std::uint32_t *)packet)[3] = network::HostToNetwork(transaction_id_);                             // Offset 12 : 32-bit transaction ID
    }

    void UDPTrackerConnection::CreateAnnounceRequest()
    {
        transaction_id_ = GenerateTransactionId();
        ResizeBuffer(ANNOUNCE_REQUEST_SIZE);

        CreateUDPAnnounceRequest(buffer_, *parameters_, transaction_id_, connection_id_.GetId());
    }

    UDPTrackerConnection::SendResult UDPTrackerConnection::Send()
    {
        assert(current_buffer_position_ < buffer_.size());

        int length = static_cast<int>(buffer_.size() - current_buffer_position_);
        const network::Socket::Result result = socket_.Send(buffer_.data() + current_buffer_position_, length);
        if (result == network::Socket::Result::Error)
            return SendResult::Error;

        current_buffer_position_ += length;
        if (current_buffer_position_ < buffer_.size())
            return SendResult::Unfinished;

        timeout_time_ = GetTimeoutTime(nr_timeouts_);
        return SendResult::Finished;
    }

    UDPTrackerConnection::ReceiveResult UDPTrackerConnection::Receive()
    {
        if (std::chrono::steady_clock::now() >= timeout_time_)
        {
            if (nr_timeouts_ == MAX_TIMEOUTS)
                return ReceiveResult::Error;

            ++nr_timeouts_;
            return ReceiveResult::Timeout;
        }

        int length = static_cast<int>(buffer_.size());
        const network::Socket::Result result = socket_.Receive(buffer_.data(), length);
        if (result == network::Socket::Result::Error)
            return ReceiveResult::Error;

        if (length == 0)
            return ReceiveResult::Unfinished;

        // We are using UDP so we receive the whole packet at once
        nr_timeouts_ = 0;
        current_buffer_position_ = length;
        return ReceiveResult::Finished;
    }

    void UDPTrackerConnection::ResizeBuffer(std::size_t size)
    {
        buffer_.resize(size);
        current_buffer_position_ = 0;
    }

    std::optional<tracker::AnnounceResponse> tracker::udp::Announce(network::URL url, std::shared_ptr<const AnnounceParameters> parameters, std::shared_ptr<std::atomic_bool> cancel)
    {
        UDPTrackerConnection connection;
        return connection.Announce(url, *parameters, cancel);
    }
}