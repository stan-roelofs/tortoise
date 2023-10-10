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

        void CreateHandshake(ByteVector &buffer, SHA1Hash info_hash, PeerId peer_id)
        {
            // handshake: <pstrlen><pstr><reserved><info_hash><peer_id>
            const auto start_size = buffer.size();
            buffer.push_back(protocol_string_size);
            util::Write(buffer, protocol_string, protocol_string_size);
            constexpr std::uint8_t reserved[8] = {0};
            util::Write(buffer, reserved, 8);
            const auto hash = info_hash.GetBytes();
            assert(hash.size() == 20);
            util::Write(buffer, hash.data(), hash.size());
            const auto peer = peer_id.Get();
            assert(peer.size() == 20);
            util::Write(buffer, peer.data(), peer.size());
            assert(buffer.size() - start_size == handshake_size);
        }

        bool ParseHandshake(ByteVector &buffer, SHA1Hash &info_hash, PeerId &peer_id)
        {
            if (buffer.size() < handshake_size)
                return false;

            if (buffer[0] != protocol_string_size)
            {
                LOG("Peer", "Invalid protocol string size: %d", buffer[0]);
                return false;
            }

            if (std::memcmp(&buffer[1], protocol_string, protocol_string_size) != 0)
            {
                LOG("Peer", "Invalid protocol string");
                return false;
            }

            info_hash = SHA1Hash(&buffer[28]);
            std::string peer_id_str(&buffer[48], &buffer[48] + 20);
            peer_id = PeerId::FromString(peer_id_str);
            buffer.erase(buffer.begin(), buffer.begin() + handshake_size);
            return true;
        }

        // TODO: update functions below

    } // namespace protocol

    Peer::Peer(PeerInfo peer_info, SHA1Hash info_hash, PeerId peer_id) : peer_info_(std::move(peer_info)),
                                                                         info_hash_(std::move(info_hash)),
                                                                         peer_id_(std::move(peer_id)),
                                                                         socket_(Socket::TransportProtocol::TCP),
                                                                         am_choking_(true),
                                                                         am_interested_(false),
                                                                         peer_choking_(true),
                                                                         peer_interested_(false),
                                                                         state_(State::Connect)
    {
        if (!socket_.Connect(peer_info_.ip, std::to_string(peer_info_.port)))
            state_ = State::Finished;

        ResetTimeout();
    }

    // TODO: while handshaking make sure we don't connect to ourselves!

    Peer::~Peer() = default;

    void Peer::Process()
    {
        CheckTimeout();

        Send();

        // TODO if not connected...

        switch (state_)
        {
        case State::Connect:
        {
            switch (socket_.GetConnectionStatus())
            {
            case Socket::Status::Pending:
                break;
            case Socket::Status::Connected:
            {
                ResetTimeout();
                state_ = State::Handshake_Send;
                LOG("Peer", "Connected to peer %s", peer_info_.ip.c_str());
                break;
            }
            case Socket::Status::Error:
            {
                LOG("Peer", "Error connecting to peer %s", peer_info_.ip.c_str());
                state_ = State::Finished;
                break;
            }
            }
            break;
        }
        case State::Handshake_Send:
        {
            LOG("Peer", "Attempting handshake with peer %s", peer_info_.ip.c_str());
            protocol::CreateHandshake(send_buffer_, info_hash_, peer_id_);
            state_ = State::Handshake_Receive;
            break;
        }
        case State::Handshake_Receive:
        {
            Receive(protocol::handshake_size);
            if (receive_buffer_.size() < protocol::handshake_size)
                break;

            SHA1Hash info_hash;
            PeerId peer_id;
            if (protocol::ParseHandshake(receive_buffer_, info_hash, peer_id))
            {
                if (info_hash != info_hash_)
                {
                    LOG("Peer", "Invalid info hash from peer %s", peer_info_.ip.c_str());
                    state_ = State::Finished;
                    break;
                }
                if (peer_info_.peer_id && peer_id != *peer_info_.peer_id)
                {
                    LOG("Peer", "Invalid peer id from peer %s", peer_info_.ip.c_str());
                    state_ = State::Finished;
                    break;
                }
                LOG("Peer", "Successfully handshaked with peer %s", peer_info_.ip.c_str());
                state_ = State::Handshake_Done;
                break;
            }
            else
            {
                LOG("Peer", "Invalid handshake from peer %s", peer_info_.ip.c_str());
                state_ = State::Finished;
                break;
            }
            break;
        }
        case State::Handshake_Done:
            break; // TODO
        case State::Finished:
            break;
        }
    }

    bool Peer::Finished() const
    {
        return state_ == State::Finished;
    }

    const PeerInfo &Peer::GetInfo() const
    {
        return peer_info_;
    }

    void Peer::CheckTimeout()
    {
        if (std::chrono::steady_clock::now() > timeout_)
        {
            LOG("Peer", "Peer %s timed out", peer_info_.ip.c_str());
            state_ = State::Finished;
        }
    }

    void Peer::ResetTimeout()
    {
        timeout_ = std::chrono::steady_clock::now() + default_timeout;
    }

    void Peer::Send()
    {
        if (send_buffer_.empty() || state_ == State::Finished)
            return;

        int length = (int)std::min(send_buffer_.size(), (std::size_t)std::numeric_limits<int>::max());
        switch (socket_.Send(send_buffer_.data(), length))
        {
        case Socket::Result::Ok:
            LOG("Peer", "Sent %d bytes to peer %s", length, peer_info_.ip.c_str());
            send_buffer_.erase(send_buffer_.begin(), send_buffer_.begin() + length);
            break;
        case Socket::Result::WouldBlock:
            break;
        case Socket::Result::Error:
            LOG("Peer", "Error sending data to peer %s", peer_info_.ip.c_str());
            state_ = State::Finished;
            break;
        }
    }

    void Peer::Receive(int length)
    {
        if (state_ == State::Connect || state_ == State::Finished)
            return;

        assert(length <= 1024); // TODO fix this properly..
        std::uint8_t chunk[1024];
        switch (socket_.Receive(chunk, length))
        {
        case Socket::Result::Ok:
            if (length == 0)
                return;

            LOG("Peer", "Received %d bytes from peer %s", length, peer_info_.ip.c_str());

            ResetTimeout();
            receive_buffer_.insert(receive_buffer_.end(), chunk, chunk + length);
            break;
        case Socket::Result::WouldBlock:
            return;
        case Socket::Result::Error:
            LOG("Peer", "Error receiving data from peer %s", peer_info_.ip.c_str());

            state_ = State::Finished;
            return;
        }
    }
}