#include "peer_connection.hpp"

#include <algorithm>
#include <cassert>
#include <random>
#include <vector>

#include <tortoise/exceptions.hpp>
#include <tortoise/metainfo.hpp>

#include "../util/log.hpp"
#include "peer_id.hpp"

namespace
{
    // TODO make this configurable
    constexpr std::chrono::seconds keep_alive_timeout(120);
    constexpr std::chrono::seconds connect_timeout(30);
    constexpr std::chrono::seconds handshake_timeout(30);

    constexpr unsigned DESIRED_REQUESTS = 5;
}

namespace tortoise
{
    namespace protocol
    {
        enum class MessageID : std::uint8_t
        {
            Choke = 0,
            Unchoke = 1,
            Interested = 2,
            NotInterested = 3,
            Have = 4,
            Bitfield = 5,
            Request = 6,
            Piece = 7,
            Cancel = 8,
            Port = 9
        };

        constexpr char protocol_string[] = "BitTorrent protocol";
        constexpr int protocol_string_size = sizeof(protocol_string) - 1;
        static_assert(protocol_string_size == 19, "Protocol string size is not 19 bytes.");
        constexpr int handshake_size = 49 + protocol_string_size;
        constexpr int length_prefix_size = 4; // The number of bytes used to prefix the length of a message
    }

    static std::uint32_t GetLengthPrefix(const ByteVector &buffer)
    {
        assert(buffer.size() >= protocol::length_prefix_size);
        if (buffer.size() < protocol::length_prefix_size)
            return 0;
        return network::NetworkToHost(util::Read<std::uint32_t>(buffer, 0));
    }

    static void CreateHandshake(ByteVector &buffer, std::array<std::uint8_t, 20> info_hash, PeerId peer_id)
    {
        // handshake: <pstrlen><pstr><reserved><info_hash><peer_id>
        const auto start_size = buffer.size();
        buffer.push_back(protocol::protocol_string_size);
        util::Write(buffer, protocol::protocol_string, protocol::protocol_string_size);
        constexpr std::uint8_t reserved[8] = {0};
        util::Write(buffer, reserved, 8);
        util::Write(buffer, info_hash.data(), info_hash.size());
        const auto peer = peer_id.Get();
        assert(peer.size() == 20);
        util::Write(buffer, peer.data(), peer.size());
        assert(buffer.size() - start_size == protocol::handshake_size);
    }

    static bool ParseHandshake(ByteVector &buffer, std::array<std::uint8_t, 20> &info_hash, PeerId &peer_id)
    {
        if (buffer.size() < protocol::handshake_size)
            return false;

        if (buffer[0] != protocol::protocol_string_size)
        {
            LOG("PeerConnection", "Invalid protocol string size: %d", buffer[0]);
            return false;
        }

        if (memcmp(&buffer[1], protocol::protocol_string, protocol::protocol_string_size) != 0)
        {
            LOG("PeerConnection", "Invalid protocol string");
            return false;
        }

        std::copy(&buffer[28], &buffer[28] + 20, info_hash.begin());
        std::string peer_id_str(&buffer[48], &buffer[48] + 20);
        peer_id = PeerId::FromString(peer_id_str);
        return true;
    }

    // TODO: while handshaking make sure we don't connect to ourselves!

    PeerConnection::PeerConnection(const PeerInfo &peer_info, std::shared_ptr<const Metainfo> metainfo, PeerId peer_id)
        : peer_info_(peer_info),
          metainfo_(std::move(metainfo)),
          own_peer_id_(std::move(peer_id)),
          nr_blocks_requested_(0),
          am_choking_(true),
          am_interested_(false),
          peer_choking_(false),
          peer_interested_(false),
          can_receive_bitfield_(true),
          status_(Status::Connecting),
          socket_(network::TransportProtocol::TCP)
    {
        if (!metainfo_)
            throw InvalidArgumentException("Metainfo is null.");

        LOG("PeerConnection", "Connecting to peer %s:%d", peer_info_.ip.c_str(), peer_info_.port);
        if (!socket_.Connect(peer_info_.ip, std::to_string(peer_info_.port)))
        {
            LOG("PeerConnection", "Error connecting to peer %s:%d", peer_info_.ip.c_str(), peer_info_.port);
            status_ = Status::Finished;
            return;
        }

        SetTimeout(connect_timeout);
    }

    PeerConnection::~PeerConnection() = default;

    void PeerConnection::Process()
    {
        if (status_ == Status::Finished)
            return; // We are done

        if (CheckTimeout())
            return;

        if (status_ == Status::Connecting && !Connect())
            return; // We are still connecting

        if (!Send())
        {
            Error(std::string("Error while sending data to peer ") + peer_info_.ip);
            return;
        }
        if (!Receive())
        {
            Error(std::string("Error while receiving data from peer ") + peer_info_.ip);
            return;
        }

        HandleMessages();
    }

    PeerConnection::Status PeerConnection::GetStatus() const
    {
        return status_;
    }

    const PeerInfo &PeerConnection::GetPeerInfo() const
    {
        return peer_info_;
    }

    // Message handlers
    bool PeerConnection::HandleMessages()
    {
        if (status_ == Status::Handshaking)
        {
            if (receive_buffer_.size() < protocol::handshake_size)
                return true; // Not enough data, wait...

            std::array<std::uint8_t, 20> info_hash;
            PeerId peer_id;
            if (!ParseHandshake(receive_buffer_, info_hash, peer_id))
            {
                Error(std::string("Invalid handshake from peer ") + peer_info_.ip);
                return false;
            }
            if (info_hash != metainfo_->info_hash)
            {
                Error(std::string("Invalid info hash from peer ") + peer_info_.ip);
                return false;
            }
            if (peer_info_.peer_id && peer_id != *peer_info_.peer_id)
            {
                Error(std::string("Received invalid peer id from peer ") + peer_info_.ip);
                return false;
            }
            LOG("PeerConnection", "Successfully handshaked with peer %s", peer_info_.ip.c_str());

            ShiftBuffer(protocol::handshake_size);
            status_ = Status::Connected;
        }

        if (status_ != Status::Connected)
            return false;

        // Handle messages as long as there is a complete message in the buffer
        while ((receive_buffer_.size() >= protocol::length_prefix_size) && ((receive_buffer_.size() - protocol::length_prefix_size) >= GetLengthPrefix(receive_buffer_)))
        {
            const auto length = GetLengthPrefix(receive_buffer_);
            if (length > 0)
            {
                switch (static_cast<protocol::MessageID>(receive_buffer_[protocol::length_prefix_size]))
                {
                case protocol::MessageID::Choke:
                    OnMessageChoke();
                    break;
                case protocol::MessageID::Unchoke:
                    OnMessageUnchoke();
                    break;
                case protocol::MessageID::Interested:
                    OnMessageInterested();
                    break;
                case protocol::MessageID::NotInterested:
                    OnMessageNotInterested();
                    break;
                case protocol::MessageID::Have:
                    OnMessageHave(network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 1)));
                    break;
                case protocol::MessageID::Bitfield:
                {
                    OnMessageBitfield(ByteVector(receive_buffer_.begin() + protocol::length_prefix_size + 1 /* skip message id */,
                                                 receive_buffer_.begin() + protocol::length_prefix_size + length));
                    break;
                }
                case protocol::MessageID::Request:
                {
                    const auto piece_index = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 1));
                    const auto begin = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 5));
                    const auto requested_length = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 9));
                    OnMessageRequest(piece_index, begin, requested_length);
                    break;
                }
                case protocol::MessageID::Piece:
                {
                    const auto piece_index = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 1));
                    const auto begin = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 5));
                    const auto block = ByteVector(receive_buffer_.begin() + protocol::length_prefix_size + 9 /* skip message id, piece index and begin */,
                                                  receive_buffer_.begin() + protocol::length_prefix_size + length);
                    OnMessagePiece(piece_index, begin, block);
                    break;
                }
                case protocol::MessageID::Cancel:
                {
                    const auto piece_index = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 1));
                    const auto begin = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 5));
                    const auto requested_length = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 9));
                    OnMessageCancel(piece_index, begin, requested_length);
                    break;
                }
                case protocol::MessageID::Port:
                {
                    const auto port = network::HostToNetwork(util::Read<std::uint16_t>(receive_buffer_, protocol::length_prefix_size + 1));
                    OnMessagePort(port);
                    break;
                }
                default:
                    Error(std::string("Received invalid message from peer ") + peer_info_.ip);
                    return false;
                }
            }

            can_receive_bitfield_ = false; // We have received a message, so we can't receive a bitfield anymore
            ShiftBuffer(protocol::length_prefix_size + length);
        }

        return true;
    }

    void PeerConnection::ShiftBuffer(std::size_t amount)
    {
        assert(amount <= receive_buffer_.size());
        if (amount > receive_buffer_.size())
            amount = receive_buffer_.size();
        receive_buffer_.erase(receive_buffer_.begin(), receive_buffer_.begin() + amount);
    }

    void PeerConnection::Error(const std::string &reason)
    {
        LOG("PeerConnection", "%s", reason.c_str());
        (void)reason;
        status_ = Status::Finished;
    }

    bool PeerConnection::Connect()
    {
        assert(status_ == Status::Connecting);
        if (status_ != Status::Connecting)
            return false;

        switch (socket_.GetConnectionStatus())
        {
        case network::Socket::Status::Pending:
            return false;
        case network::Socket::Status::Connected:
        {
            SetTimeout(handshake_timeout);
            status_ = Status::Handshaking;
            CreateHandshake(send_buffer_, metainfo_->info_hash, own_peer_id_);
            LOG("PeerConnection", "Connected to peer %s", peer_info_.ip.c_str());
            return true;
        }
        case network::Socket::Status::Error:
        {
            Error(std::string("Error connecting to peer ") + peer_info_.ip);
            return false;
        }
        }

        return false; // Not reached
    }

    void PeerConnection::SetTimeout(std::chrono::seconds timeout)
    {
        timeout_ = std::chrono::steady_clock::now() + timeout;
    }

    bool PeerConnection::CheckTimeout()
    {
        if (std::chrono::steady_clock::now() <= timeout_)
            return false;

        LOG("PeerConnection", "Peer %s timed out", peer_info_.ip.c_str());
        status_ = Status::Finished;
        return true;
    }

    bool PeerConnection::Send()
    {
        if (send_buffer_.empty())
            return true;

        int length = (int)std::min(send_buffer_.size(), (std::size_t)std::numeric_limits<int>::max());
        switch (socket_.Send(send_buffer_.data(), length))
        {
        case network::Socket::Result::Ok:
            LOG("PeerConnection", "Sent %d bytes to peer %s", length, peer_info_.ip.c_str());
            send_buffer_.erase(send_buffer_.begin(), send_buffer_.begin() + length);
            return true;
        case network::Socket::Result::WouldBlock:
            return true;
        case network::Socket::Result::Error:
            return false;
            break;
        }

        return true;
    }

    bool PeerConnection::Receive()
    {
        int length = 1024;
        std::uint8_t chunk[1024];
        switch (socket_.Receive(chunk, length))
        {
        case network::Socket::Result::Ok:
            if (length != 0)
            {
                LOG("PeerConnection", "Received %d bytes from peer %s", length, peer_info_.ip.c_str());

                SetTimeout(keep_alive_timeout);
                receive_buffer_.insert(receive_buffer_.end(), chunk, chunk + length);
            }
            return true;
        case network::Socket::Result::WouldBlock:
            return true;
        case network::Socket::Result::Error:
            return false;
        }

        return true;
    }

    void PeerConnection::OnMessageChoke()
    {
        LOG("PeerConnection", "Peer %s choked us", peer_info_.ip.c_str());
        peer_choking_ = true;
    }

    void PeerConnection::OnMessageUnchoke()
    {
        LOG("PeerConnection", "Peer %s unchoked us", peer_info_.ip.c_str());
        peer_choking_ = false;
    }

    void PeerConnection::OnMessageInterested()
    {
        LOG("PeerConnection", "Peer %s is interested", peer_info_.ip.c_str());
        peer_interested_ = true;
    }

    void PeerConnection::OnMessageNotInterested()
    {
        LOG("PeerConnection", "Peer %s is not interested", peer_info_.ip.c_str());
        peer_interested_ = false;
    }

    void PeerConnection::OnMessageHave(std::uint32_t piece_index)
    {
        LOG("PeerConnection", "Peer %s has piece %d", peer_info_.ip.c_str(), piece_index);
    }

    void PeerConnection::OnMessageBitfield(const ByteVector &bitfield)
    {
        if (!can_receive_bitfield_)
            return;

        const std::size_t expected_bitfield_size = (metainfo_->pieces.size() + 7) / 8;
        if (bitfield.size() != expected_bitfield_size)
        {
            Error(std::string("Peer ") + peer_info_.ip + " sent invalid bitfield");
            return;
        }

        LOG("PeerConnection", "Peer %s sent bitfield", peer_info_.ip.c_str());

        for (std::uint32_t byte = 0; byte < bitfield.size(); ++byte)
        {
            for (std::uint32_t bit = 0; bit < 8; ++bit)
            {
                if (bitfield[byte] & (1 << (7 - bit)))
                {
                    //const std::uint32_t piece_index = (byte * 8) + bit;
                }
            }
        }
    }

    void PeerConnection::OnMessageRequest(std::uint32_t index, std::uint32_t begin, std::uint32_t length)
    {
        LOG("PeerConnection", "Peer %s requested piece %d, begin %d, length %d", peer_info_.ip.c_str(), index, begin, length);
    }

    void PeerConnection::OnMessagePiece(std::uint32_t index, std::uint32_t begin, const ByteVector &piece)
    {
        LOG("PeerConnection", "Peer %s sent piece %d, begin %d, length %d", peer_info_.ip.c_str(), index, begin, (uint32_t)piece.size());
    }

    void PeerConnection::OnMessageCancel(std::uint32_t index, std::uint32_t begin, std::uint32_t length)
    {
        LOG("PeerConnection", "Peer %s cancelled request for piece %d, begin %d, length %d", peer_info_.ip.c_str(), index, begin, length);
    }

    void PeerConnection::OnMessagePort(std::uint16_t port)
    {
        LOG("PeerConnection", "Peer %s sent port %d", peer_info_.ip.c_str(), port);
    }
}