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
    // TODO handle errors (action 3)

    namespace
    {
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

        void CreateUDPAnnounceRequest(std::vector<std::uint8_t> &buffer, const AnnounceParameters &parameters,
                                      std::uint32_t transaction_id, std::uint64_t connection_id)
        {
            buffer.resize(ANNOUNCE_REQUEST_SIZE);
            std::uint8_t *packet = buffer.data();
            *(std::uint64_t *)(packet + 0) = Socket::ToNetworkByteOrder(connection_id);
            *(std::uint32_t *)(packet + 8) = Socket::ToNetworkByteOrder(static_cast<std::uint32_t>(Action::Announce));
            *(std::uint32_t *)(packet + 12) = Socket::ToNetworkByteOrder(transaction_id);
            std::memcpy(packet + 16, parameters.info_hash.GetBytes().data(), 20);
            std::memcpy(packet + 36, parameters.peer_id.Get().data(), 20);
            *(std::uint64_t *)(packet + 56) = Socket::ToNetworkByteOrder(parameters.downloaded);
            *(std::uint64_t *)(packet + 64) = Socket::ToNetworkByteOrder(parameters.left);
            *(std::uint64_t *)(packet + 72) = Socket::ToNetworkByteOrder(parameters.uploaded);
            *(std::uint32_t *)(packet + 80) = Socket::ToNetworkByteOrder(static_cast<std::uint32_t>(parameters.event));
            if (!parameters.ip || parameters.ip->IsIPv6()) // the IP address field in the request remains 32bits wide which makes this field not usable under IPv6.
            {
                *(std::uint32_t *)(packet + 84) = 0;
            }
            else
            {
                const auto ip_address = parameters.ip->ToVector();
                assert(ip_address.size() == 4);
                *(std::uint32_t *)(packet + 84) = Socket::ToNetworkByteOrder(*(std::uint32_t *)ip_address.data());
            }
            *(std::uint32_t *)(packet + 88) = parameters.key ? Socket::ToNetworkByteOrder(parameters.key.value()) : 0;
            *(std::uint32_t *)(packet + 92) = parameters.numwant ? Socket::ToNetworkByteOrder(parameters.numwant.value()) : 0;
            *(std::uint16_t *)(packet + 96) = Socket::ToNetworkByteOrder(parameters.port);
            *(std::uint16_t *)(packet + 98) = Socket::ToNetworkByteOrder((uint16_t)0); // extensions
        }

        std::optional<AnnounceResponse> ParseResponse(const std::uint8_t *packet, const int packet_size, Socket::InternetProtocol protocol)
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

            AnnounceResponse result;

            result.interval = Socket::FromNetworkByteOrder(((std::uint32_t *)packet)[2]);
            result.incomplete = Socket::FromNetworkByteOrder(((std::uint32_t *)packet)[3]);
            result.complete = Socket::FromNetworkByteOrder(((std::uint32_t *)packet)[4]);

            const int nr_peers = (packet_size - ANNOUNCE_RESPONSE_MINIMUM_SIZE) / stride_size;
            for (int i = 0; i < nr_peers; i++)
            {
                const std::uint8_t *peer_start = packet + ANNOUNCE_RESPONSE_MINIMUM_SIZE + i * stride_size;
                if (stride_size == 6)
                {
                    const IPAddress ip_address(IPAddress::ipv4_address_t{peer_start[0], peer_start[1], peer_start[2], peer_start[3]});
                    const std::uint16_t port = Socket::FromNetworkByteOrder(((std::uint16_t *)peer_start)[2]);
                    result.peers.push_back(AnnounceResponse::PeerInfo{ip_address.ToString(), port});
                }
                else if (stride_size == 18)
                {
                    const IPAddress ip_address(IPAddress::ipv6_address_t{
                        peer_start[0], peer_start[1], peer_start[2], peer_start[3],
                        peer_start[4], peer_start[5], peer_start[6], peer_start[7],
                        peer_start[8], peer_start[9], peer_start[10], peer_start[11],
                        peer_start[12], peer_start[13], peer_start[14], peer_start[15]});
                    const std::uint16_t port = Socket::FromNetworkByteOrder(((std::uint16_t *)peer_start)[8]);
                    result.peers.push_back(AnnounceResponse::PeerInfo{ip_address.ToString(), port});
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

    UDPTrackerConnection::UDPTrackerConnection() : result_({false, {}}),
                                                   state_(State::Idle),
                                                   socket_(Socket::TransportProtocol::UDP),
                                                   transaction_id_(0),
                                                   current_buffer_position_(0u),
                                                   nr_timeouts_(0u)
    {
    }

    UDPTrackerConnection::~UDPTrackerConnection()
    {
        // TODO send a "stopped" event
    }

    bool UDPTrackerConnection::Announce(const URL &url, const AnnounceParameters &parameters)
    {
        if (state_ != State::Idle)
            return false;

        parameters_ = std::make_unique<AnnounceParameters>(parameters);
        if (!socket_.Connect(url.GetHost(), url.GetPort()))
        {
            LOG("UDPTrackerConnection", "failed to connect to %s:%s", url.GetHost().c_str(), url.GetPort().c_str());
            return false;
        }

        CreateConnectRequest();
        state_ = State::SendConnectRequest;

        LOG("UDPTrackerConnection", "connected to %s:%s", url.GetHost().c_str(), url.GetPort().c_str());
        return true;
    }

    TrackerConnection::Result UDPTrackerConnection::GetLastResult() const
    {
        return result_;
    }

    void UDPTrackerConnection::Cancel()
    {
        state_ = State::Idle;
    }

    bool UDPTrackerConnection::Process()
    {
        // TODO handle timeouts
        switch (state_)
        {
        case State::Idle:
            return true;
        case State::SendConnectRequest:
        {
            if (!Send())
                return false;

            LOG("UDPTrackerConnection", "sent connect request");
            ResizeBuffer(CONNECT_RESPONSE_SIZE);
            state_ = State::ReceiveConnectResponse;
            return false;
        }
        case State::ReceiveConnectResponse:
        {
            const auto receive_result = Receive();
            switch (receive_result)
            {
            case ReceiveResult::Finished:
                break;
            case ReceiveResult::Unfinished:
            case ReceiveResult::Error:
                return false;
            case ReceiveResult::Timeout:
                CreateConnectRequest();
                state_ = State::SendConnectRequest;
                return false;
            }

            const std::uint8_t *packet = buffer_.data();
            const std::uint32_t action = Socket::FromNetworkByteOrder(((std::uint32_t *)packet)[0]);
            const std::uint32_t received_transaction_id = Socket::FromNetworkByteOrder(((std::uint32_t *)packet)[1]);
            std::uint64_t connection_id = Socket::FromNetworkByteOrder(((std::uint64_t *)packet)[1]);

            if (received_transaction_id != transaction_id_)
            {
                LOG("UDPTrackerConnection", "Transaction ID mismatch.");
                return false;
            }

            if (action != static_cast<std::uint32_t>(Action::Connect))
            {
                LOG("UDPTrackerConnection", "Invalid action.");
                return false;
            }

            connection_id_ = ConnectionId(connection_id);
            LOG("UDPTrackerConnection", "Server accepted connect request. Our connection id is: %" PRId64, connection_id_.GetId());

            CreateAnnounceRequest();
            state_ = State::SendAnnounceRequest;
        }
            [[fallthrough]];
        case State::SendAnnounceRequest:
        {
            if (!connection_id_.IsValid())
            {
                CreateConnectRequest();
                state_ = State::SendConnectRequest;
                return false;
            }

            if (!Send())
                return false;

            LOG("UDPTrackerConnection", "sent announce request");
            state_ = State::ReceiveAnnounceResponse;
            ResizeBuffer(0xFFFF);
            return false;
        }
        case State::ReceiveAnnounceResponse:
        {
            const auto receive_result = Receive();
            switch (receive_result)
            {
            case ReceiveResult::Finished:
                break;
            case ReceiveResult::Unfinished:
            case ReceiveResult::Error:
                return false;
            case ReceiveResult::Timeout:
                CreateAnnounceRequest();
                state_ = State::SendAnnounceRequest;
                return false;
            }

            const std::uint8_t *packet = buffer_.data();
            const std::uint32_t action = Socket::FromNetworkByteOrder(((std::uint32_t *)packet)[0]);
            const std::uint32_t received_transaction_id = Socket::FromNetworkByteOrder(((std::uint32_t *)packet)[1]);

            if (received_transaction_id != transaction_id_)
            {
                LOG("UDPTrackerConnection", "Transaction ID mismatch.");
                return false;
            }

            if (action != static_cast<std::uint32_t>(Action::Announce))
            {
                LOG("UDPTrackerConnection", "Invalid action.");
                return false;
            }

            std::optional<AnnounceResponse> response = ParseResponse(packet, (int)current_buffer_position_, socket_.GetInternetProtocol());
            if (!response)
            {
                LOG("UDPTrackerConnection", "Failed to parse announce response.");
                return false;
            }

            result_ = Result{true, response};
            state_ = State::Idle;
            return true;
        }
        }

        return false;
    }

    void UDPTrackerConnection::CreateConnectRequest()
    {
        transaction_id_ = GenerateTransactionId();
        ResizeBuffer(CONNECT_REQUEST_SIZE);

        std::uint8_t *packet = buffer_.data();
        ((std::uint64_t *)packet)[0] = Socket::ToNetworkByteOrder(MAGIC_PROTOCOL_CONSTANT);                     // Offset 0 : 64-bit magic constant
        ((std::uint32_t *)packet)[2] = Socket::ToNetworkByteOrder(static_cast<std::uint32_t>(Action::Connect)); // Offset 8 : 32-bit action
        ((std::uint32_t *)packet)[3] = Socket::ToNetworkByteOrder(transaction_id_);                             // Offset 12 : 32-bit transaction ID
    }

    void UDPTrackerConnection::CreateAnnounceRequest()
    {
        transaction_id_ = GenerateTransactionId();
        current_buffer_position_ = 0;

        CreateUDPAnnounceRequest(buffer_, *parameters_, transaction_id_, connection_id_.GetId());
    }

    bool UDPTrackerConnection::Send()
    {
        assert(current_buffer_position_ < buffer_.size());

        int length = static_cast<int>(buffer_.size() - current_buffer_position_);
        const Socket::Result result = socket_.Send(buffer_.data() + current_buffer_position_, length);
        if (result == Socket::Result::Error)
        {
            result_ = Result{false, {}};
            state_ = State::Idle;
            return false;
        }

        current_buffer_position_ += length;
        if (current_buffer_position_ < buffer_.size())
            return false;

        timeout_time_ = GetTimeoutTime(nr_timeouts_);
        return true;
    }

    UDPTrackerConnection::ReceiveResult UDPTrackerConnection::Receive()
    {
        if (std::chrono::steady_clock::now() >= timeout_time_)
        {
            if (nr_timeouts_ == MAX_TIMEOUTS)
            {
                result_ = Result{false, {}};
                state_ = State::Idle;
                return ReceiveResult::Error;
            }

            ++nr_timeouts_;
            return ReceiveResult::Timeout;
        }

        int length = static_cast<int>(buffer_.size());
        const Socket::Result result = socket_.Receive(buffer_.data(), length);
        if (result == Socket::Result::Error)
        {
            result_ = Result{false, {}};
            state_ = State::Idle;
            return ReceiveResult::Error;
        }

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
}