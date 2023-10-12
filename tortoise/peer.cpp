#include "peer.hpp"

#include <algorithm>
#include <cassert>
#include <vector>

#include <tortoise/sha1_hash.hpp>

#include "log.hpp"
#include "peer_id.hpp"

namespace
{
    constexpr std::chrono::seconds keep_alive_interval(120);
    constexpr std::chrono::seconds default_timeout(30);
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

    Peer::Peer(PeerInfo peer_info, SHA1Hash info_hash, PeerId peer_id) : peer_info_(std::move(peer_info)),
                                                                         info_hash_(std::move(info_hash)),
                                                                         peer_id_(std::move(peer_id)),
                                                                         am_choking_(true),
                                                                         am_interested_(false),
                                                                         peer_choking_(true),
                                                                         peer_interested_(false),
                                                                         state_(State::Connecting),
                                                                         socket_(network::TransportProtocol::TCP)

    {
        if (!socket_.Connect(peer_info_.ip, std::to_string(peer_info_.port)))
            state_ = State::Finished;

        ResetTimeout();
    }

    // TODO: while handshaking make sure we don't connect to ourselves!

    Peer::~Peer() = default;

    bool Peer::Process()
    {
        if (state_ == State::Finished)
            return false; // We are done

        if (CheckTimeout())
            return false;

        if (state_ == State::Connecting && !Connect())
            return true; // We are still connecting

        if (!Send())
        {
            Error(std::string("Error while sending data to peer ") + peer_info_.ip);
            return false;
        }
        if (!Receive())
        {
            Error(std::string("Error while receiving data from peer ") + peer_info_.ip);
            return false;
        }

        HandleMessages();
        return true;
    }

    const PeerInfo &Peer::GetInfo() const
    {
        return peer_info_;
    }

    bool Peer::CheckTimeout()
    {
        if (std::chrono::steady_clock::now() <= timeout_)
            return false;

        LOG("Peer", "Peer %s timed out", peer_info_.ip.c_str());
        state_ = State::Finished;
        return true;
    }

    void Peer::ResetTimeout()
    {
        timeout_ = std::chrono::steady_clock::now() + default_timeout;
    }

    bool Peer::Send()
    {
        if (send_buffer_.empty())
            return true;

        int length = (int)std::min(send_buffer_.size(), (std::size_t)std::numeric_limits<int>::max());
        switch (socket_.Send(send_buffer_.data(), length))
        {
        case Socket::Result::Ok:
            LOG("Peer", "Sent %d bytes to peer %s", length, peer_info_.ip.c_str());
            send_buffer_.erase(send_buffer_.begin(), send_buffer_.begin() + length);
            return true;
        case Socket::Result::WouldBlock:
            return true;
        case Socket::Result::Error:
            return false;
            break;
        }

        return true;
    }

    bool Peer::Receive()
    {
        int length = 1024;
        std::uint8_t chunk[1024];
        switch (socket_.Receive(chunk, length))
        {
        case Socket::Result::Ok:
            if (length != 0)
            {
                LOG("Peer", "Received %d bytes from peer %s", length, peer_info_.ip.c_str());

                ResetTimeout();
                receive_buffer_.insert(receive_buffer_.end(), chunk, chunk + length);
            }
            return true;
        case Socket::Result::WouldBlock:
            return true;
        case Socket::Result::Error:
            return false;
        }

        return true;
    }

    static void CreateHandshake(ByteVector &buffer, SHA1Hash info_hash, PeerId peer_id)
    {
        // handshake: <pstrlen><pstr><reserved><info_hash><peer_id>
        const auto start_size = buffer.size();
        buffer.push_back(protocol::protocol_string_size);
        util::Write(buffer, protocol::protocol_string, protocol::protocol_string_size);
        constexpr std::uint8_t reserved[8] = {0};
        util::Write(buffer, reserved, 8);
        const auto hash = info_hash.GetBytes();
        assert(hash.size() == 20);
        util::Write(buffer, hash.data(), hash.size());
        const auto peer = peer_id.Get();
        assert(peer.size() == 20);
        util::Write(buffer, peer.data(), peer.size());
        assert(buffer.size() - start_size == protocol::handshake_size);
    }

    bool Peer::Connect()
    {
        assert(state_ == State::Connecting);
        if (state_ != State::Connecting)
            return false;

        switch (socket_.GetConnectionStatus())
        {
        case Socket::Status::Pending:
            return false;
        case Socket::Status::Connected:
        {
            ResetTimeout();
            state_ = State::Handshaking;
            CreateHandshake(send_buffer_, info_hash_, peer_id_);
            LOG("Peer", "Connected to peer %s", peer_info_.ip.c_str());
            return true;
        }
        case Socket::Status::Error:
        {
            Error(std::string("Error connecting to peer ") + peer_info_.ip);
            return false;
        }
        }

        return false; // Not reached
    }

    static bool ParseHandshake(ByteVector &buffer, SHA1Hash &info_hash, PeerId &peer_id)
    {
        if (buffer.size() < protocol::handshake_size)
            return false;

        if (buffer[0] != protocol::protocol_string_size)
        {
            LOG("Peer", "Invalid protocol string size: %d", buffer[0]);
            return false;
        }

        if (memcmp(&buffer[1], protocol::protocol_string, protocol::protocol_string_size) != 0)
        {
            LOG("Peer", "Invalid protocol string");
            return false;
        }

        info_hash = SHA1Hash(&buffer[28]);
        std::string peer_id_str(&buffer[48], &buffer[48] + 20);
        peer_id = PeerId::FromString(peer_id_str);
        return true;
    }

    bool Peer::HandleMessages()
    {
        if (state_ == State::Handshaking)
        {
            if (receive_buffer_.size() < protocol::handshake_size)
                return true; // Not enough data, wait...

            SHA1Hash info_hash;
            PeerId peer_id;
            if (!ParseHandshake(receive_buffer_, info_hash, peer_id))
            {
                Error(std::string("Invalid handshake from peer ") + peer_info_.ip);
                return false;
            }

            if (info_hash != info_hash_)
            {
                Error(std::string("Invalid info hash from peer ") + peer_info_.ip);
                return false;
            }
            if (peer_info_.peer_id && peer_id != *peer_info_.peer_id)
            {
                Error(std::string("Received invalid peer id from peer ") + peer_info_.ip);
                return false;
            }
            LOG("Peer", "Successfully handshaked with peer %s", peer_info_.ip.c_str());
            ShiftBuffer(protocol::handshake_size);
            state_ = State::Connected;
        }

        assert(state_ == State::Connected);
        if (state_ != State::Connected)
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
                    OnChoke();
                    break;
                case protocol::MessageID::Unchoke:
                    OnUnchoke();
                    break;
                case protocol::MessageID::Interested:
                    OnInterested();
                    break;
                case protocol::MessageID::NotInterested:
                    OnNotInterested();
                    break;
                case protocol::MessageID::Have:
                    OnHave(network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 1)));
                    break;
                case protocol::MessageID::Bitfield:
                {
                    OnBitfield(ByteVector(receive_buffer_.begin() + protocol::length_prefix_size + 1 /* skip message id */,
                                          receive_buffer_.begin() + protocol::length_prefix_size + length));
                    break;
                }
                case protocol::MessageID::Request:
                {
                    const auto piece_index = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 1));
                    const auto begin = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 5));
                    const auto requested_length = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 9));
                    OnRequest(piece_index, begin, requested_length);
                    break;
                }
                case protocol::MessageID::Piece:
                {
                    const auto piece_index = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 1));
                    const auto begin = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 5));
                    const auto block = ByteVector(receive_buffer_.begin() + protocol::length_prefix_size + 9 /* skip message id, piece index and begin */,
                                                  receive_buffer_.begin() + protocol::length_prefix_size + length);
                    OnPiece(piece_index, begin, block);
                    break;
                }
                case protocol::MessageID::Cancel:
                {
                    const auto piece_index = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 1));
                    const auto begin = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 5));
                    const auto requested_length = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 9));
                    OnCancel(piece_index, begin, requested_length);
                    break;
                }
                case protocol::MessageID::Port:
                {
                    const auto port = network::HostToNetwork(util::Read<std::uint16_t>(receive_buffer_, protocol::length_prefix_size + 1));
                    OnPort(port);
                    break;
                }
                default:
                    Error(std::string("Received invalid message from peer ") + peer_info_.ip);
                    return false;
                }
            }
            ShiftBuffer(protocol::length_prefix_size + length);
        }

        return true;
    }

    void Peer::OnChoke()
    {
        LOG("Peer", "Peer %s choked us", peer_info_.ip.c_str());
        peer_choking_ = true;
    }

    void Peer::OnUnchoke()
    {
        LOG("Peer", "Peer %s unchoked us", peer_info_.ip.c_str());
        peer_choking_ = false;
    }

    void Peer::OnInterested()
    {
        LOG("Peer", "Peer %s is interested", peer_info_.ip.c_str());
        peer_interested_ = true;
    }

    void Peer::OnNotInterested()
    {
        LOG("Peer", "Peer %s is not interested", peer_info_.ip.c_str());
        peer_interested_ = false;
    }

    void Peer::OnHave(std::uint32_t piece_index)
    {
        LOG("Peer", "Peer %s has piece %u", peer_info_.ip.c_str(), piece_index);
        // TODO
        (void)piece_index;
    }

    void Peer::OnBitfield(const ByteVector &bitfield)
    {
        LOG("Peer", "Peer %s sent bitfield", peer_info_.ip.c_str());
        // TODO
        (void)bitfield;
    }

    void Peer::OnRequest(std::uint32_t piece_index, std::uint32_t begin, std::uint32_t length)
    {
        LOG("Peer", "Peer %s requested piece %u", peer_info_.ip.c_str(), piece_index);
        // TODO
        (void)piece_index;
        (void)begin;
        (void)length;
    }

    void Peer::OnPiece(std::uint32_t piece_index, std::uint32_t begin, const ByteVector &block)
    {
        LOG("Peer", "Peer %s sent piece %u", peer_info_.ip.c_str(), piece_index);
        // TODO
        (void)piece_index;
        (void)begin;
        (void)block;
    }

    void Peer::OnCancel(std::uint32_t piece_index, std::uint32_t begin, std::uint32_t length)
    {
        LOG("Peer", "Peer %s cancelled piece %u", peer_info_.ip.c_str(), piece_index);
        // TODO
        (void)piece_index;
        (void)begin;
        (void)length;
    }

    void Peer::OnPort(std::uint16_t port)
    {
        LOG("Peer", "Peer %s sent port %u", peer_info_.ip.c_str(), port);
        // TODO
        (void)port;
    }

    void Peer::ShiftBuffer(std::size_t amount)
    {
        assert(amount <= receive_buffer_.size());
        if (amount > receive_buffer_.size())
            amount = receive_buffer_.size();
        receive_buffer_.erase(receive_buffer_.begin(), receive_buffer_.begin() + amount);
    }

    void Peer::Error(const std::string &reason)
    {
        LOG("Peer", "%s", reason.c_str());
        state_ = State::Finished;
    }
}