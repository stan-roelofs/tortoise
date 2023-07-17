#include "peer.hpp"

#include <cassert>
#include <vector>

#include <tortoise/sha1_hash.hpp>

#include "log.hpp"
#include "peer_id.hpp"

namespace
{
    constexpr std::chrono::seconds keep_alive_interval(120);
}

namespace tortoise
{
    namespace messages
    {
        enum class ID : std::uint8_t
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

        std::vector<std::uint8_t> HandShake(SHA1Hash info_hash, PeerId peer_id)
        {
            // handshake: <pstrlen><pstr><reserved><info_hash><peer_id>
            std::vector<std::uint8_t> handshake;
            handshake.resize(68);
            auto *buffer = handshake.data();
            *((std::uint32_t *)buffer[0]) = Socket::ToNetworkByteOrder(static_cast<std::uint32_t>(19));
            constexpr char protocol_string[] = {'B', 'i', 't', 'T', 'o', 'r', 'r', 'e', 'n', 't', ' ', 'p', 'r', 'o', 't', 'o', 'c', 'o', 'l'};
            std::memcpy(buffer + 1, protocol_string, sizeof(protocol_string));
            static_assert(sizeof(protocol_string) == 19, "Protocol string size is not 19");
            const auto hash = info_hash.GetBytes();
            std::memcpy(buffer + 20, hash.data(), hash.size());
            assert(hash.size() == 20);
            const auto peer = peer_id.Get();
            assert(peer.size() == 20);
            std::memcpy(buffer + 40, peer.data(), peer.size());
            assert(handshake.size() == 68);
            return handshake;
        }

        /*! \brief The keep-alive message is a message with zero bytes, specified with the length prefix set to zero. Peers may close a connection if they receive no messages
         * for a certain period of time, so a keep-alive message must be sent to maintain the connection if no messages have been sent for a given amount of time.
         */
        std::vector<std::uint8_t> KeepAlive()
        {
            return std::vector<std::uint8_t>{0, 0, 0, 0};
        }

        //! \brief The choke message is fixed length, and is used to notify the peer that the sender is choking it.
        std::vector<std::uint8_t> Choke()
        {
            std::vector<std::uint8_t> message;
            message.resize(5);
            auto *buffer = message.data();
            *((std::uint32_t *)buffer[0]) = Socket::ToNetworkByteOrder(static_cast<std::uint32_t>(1)); // length prefix
            buffer[4] = static_cast<std::uint8_t>(ID::Choke);
            assert(message.size() == 5);
            return message;
        }

        //! \brief The unchoke message is fixed length, and is used to notify the peer that the sender is no longer choking.
        std::vector<std::uint8_t> Unchoke()
        {
            std::vector<std::uint8_t> message;
            message.resize(5);
            auto *buffer = message.data();
            *((std::uint32_t *)buffer[0]) = Socket::ToNetworkByteOrder(static_cast<std::uint32_t>(1)); // length prefix
            buffer[4] = static_cast<std::uint8_t>(ID::Unchoke);
            return message;
        }

        //! \brief The interested message is fixed length, and is used to notify the peer that the sender is interested in downloading a piece.
        std::vector<std::uint8_t> Interested()
        {
            std::vector<std::uint8_t> message;
            message.resize(5);
            auto *buffer = message.data();
            *((std::uint32_t *)buffer[0]) = Socket::ToNetworkByteOrder(static_cast<std::uint32_t>(1)); // length prefix
            buffer[4] = static_cast<std::uint8_t>(ID::Interested);
            return message;
        }

        //! \brief The not interested message is fixed length, and is used to notify the peer that the sender is no longer interested in downloading a piece.
        std::vector<std::uint8_t> NotInterested()
        {
            std::vector<std::uint8_t> message;
            message.resize(5);
            auto *buffer = message.data();
            *((std::uint32_t *)buffer[0]) = Socket::ToNetworkByteOrder(static_cast<std::uint32_t>(1)); // length prefix
            buffer[4] = static_cast<std::uint8_t>(ID::NotInterested);
            return message;
        }

        /*! \brief The have message is fixed length, and is used to notify peers that the sender has downloaded a piece.
         *  \param piece_index zero-based piece index
         */
        std::vector<std::uint8_t> Have(std::uint32_t piece_index)
        {
            std::vector<std::uint8_t> message;
            message.resize(9);
            auto *buffer = message.data();
            *((std::uint32_t *)buffer[0]) = Socket::ToNetworkByteOrder(static_cast<std::uint32_t>(5)); // length prefix
            buffer[4] = static_cast<std::uint8_t>(ID::Have);
            *((std::uint32_t *)buffer[5]) = Socket::ToNetworkByteOrder(piece_index);
            return message;
        }

        /*! \brief The bitfield message may only be sent immediately after the handshaking sequence is completed, and before any other messages are sent.
         *  It is optional, and need not be sent if a client has no pieces. The message consists of a one-byte id field followed by a variable-length bitfield.
         *  \param bitfield bitfield A bitfield that represents the pieces that have been succesfully downloaded. The high bit in the first byte corresponds to piece index 0.
         *  Bits that are cleared indicated a missing piece, and set bits indicate a valid and available piece. Spare bits at the end are set to zero.
         */
        std::vector<std::uint8_t> Bitfield(const std::vector<std::uint8_t> &bitfield)
        {
            std::vector<std::uint8_t> message;
            message.resize(5 + bitfield.size());
            auto *buffer = message.data();
            *((std::uint32_t *)buffer[0]) = Socket::ToNetworkByteOrder(static_cast<std::uint32_t>(1 + bitfield.size())); // length prefix
            buffer[4] = static_cast<std::uint8_t>(ID::Bitfield);
            message.insert(message.begin() + 5, bitfield.begin(), bitfield.end());
            return message;
        }

        /*! \brief The request message is fixed length, and is used to request a block.
         * \param index zero-based piece index
         * \param begin zero-based byte offset within the piece
         * \param length requested length
         */
        std::vector<std::uint8_t> Request(std::uint32_t index, std::uint32_t begin, std::uint32_t length)
        {
            std::vector<std::uint8_t> message;
            message.resize(17);
            auto *buffer = message.data();
            *((std::uint32_t *)buffer[0]) = Socket::ToNetworkByteOrder(static_cast<std::uint32_t>(13)); // length
            buffer[4] = static_cast<std::uint8_t>(ID::Request);                                         // id
            *((std::uint32_t *)buffer[5]) = Socket::ToNetworkByteOrder(index);                          // zero-based piece index
            *((std::uint32_t *)buffer[9]) = Socket::ToNetworkByteOrder(begin);                          // zero-based byte offset within the piece
            *((std::uint32_t *)buffer[13]) = Socket::ToNetworkByteOrder(length);                        // requested length
            return message;
        }

        /*! \brief The piece message is variable length, where X is the length of the block.
         *  \param index zero-based piece index
         *  \param begin zero-based byte offset within the piece
         *  \param block block of data, which is a subset of the piece specified by index.
         */
        std::vector<std::uint8_t> Piece(std::uint32_t index, std::uint32_t begin, const std::vector<std::uint8_t> &block)
        {
            std::vector<std::uint8_t> message;
            message.resize(13 + block.size());
            auto *buffer = message.data();
            *((std::uint32_t *)buffer[0]) = Socket::ToNetworkByteOrder(static_cast<std::uint32_t>(9 + block.size())); // length
            buffer[4] = static_cast<std::uint8_t>(ID::Piece);                                                         // id
            *((std::uint32_t *)buffer[5]) = Socket::ToNetworkByteOrder(index);                                        // zero-based piece index
            *((std::uint32_t *)buffer[9]) = Socket::ToNetworkByteOrder(begin);                                        // zero-based byte offset within the piece
            message.insert(message.begin() + 13, block.begin(), block.end());
            return message;
        }

        /*! \brief The cancel message is fixed length, and is used to cancel block requests.
         *  \param index zero-based piece index
         *  \param begin zero-based byte offset within the piece
         *  \param length requested length
         */
        std::vector<std::uint8_t> Cancel(std::uint32_t index, std::uint32_t begin, std::uint32_t length)
        {
            std::vector<std::uint8_t> message;
            message.reserve(17);
            auto *buffer = message.data();
            *((std::uint32_t *)buffer[0]) = Socket::ToNetworkByteOrder(static_cast<std::uint32_t>(13)); // length
            buffer[4] = static_cast<std::uint8_t>(ID::Cancel);                                          // id
            *((std::uint32_t *)buffer[5]) = Socket::ToNetworkByteOrder(index);                          // zero-based piece index
            *((std::uint32_t *)buffer[9]) = Socket::ToNetworkByteOrder(begin);                          // zero-based byte offset within the piece
            *((std::uint32_t *)buffer[13]) = Socket::ToNetworkByteOrder(length);                        // requested length
            return message;
        }
    }

    Peer::Peer(const PeerInfo &peer_info) : peer_info_(peer_info), socket_(Socket::TransportProtocol::TCP), am_choking(true), am_interested(false), peer_choking(true), peer_interested(false), state_(State::Connect)
    {
        if (!socket_.Connect(peer_info_.ip, std::to_string(peer_info_.port)))
            state_ = State::Finished;
    }

    // TODO: while handshaking make sure we don't connect to ourselves!

    Peer::~Peer() = default;

    void Peer::Process()
    {
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
                state_ = State::Handshake;
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
        case State::Handshake:
        {
            break;
        }
        case State::Finished:
            break;
        }
    }

    bool Peer::Finished() const
    {
        return state_ == State::Finished;
    }
}