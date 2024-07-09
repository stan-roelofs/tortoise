#ifndef TORTOISE_PEER_CONNECTION_HPP
#define TORTOISE_PEER_CONNECTION_HPP

#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <thread>

#include <tortoise/metainfo.hpp>

#include "peer_info.hpp"
#include "piece_manager.hpp"

#include "../network/socket.hpp"
#include "../util/util.hpp"

namespace tortoise
{
    class PeerConnection
    {
    public:
        enum class Status
        {
            Connecting,
            Connected,
            Handshaking,
            Finished
        };

        /*!
         * \brief Creates a new peer.
         * \param peer_info The peer info.
         * \param info_hash The info hash of the torrent, used for the handshake.
         * \param peer_id The peer id of this client, used for the handshake.
         */
        PeerConnection(const PeerInfo &peer_info, std::shared_ptr<const Metainfo> metainfo, PeerId peer_id);
        ~PeerConnection();

        Status GetStatus() const;
        const PeerInfo &GetPeerInfo() const;

        void Process();

    private:
        bool HandleMessages();
        void ShiftBuffer(std::size_t amount);
        void Error(const std::string &reason);
        bool Connect();
        void SetTimeout(std::chrono::seconds timeout);
        bool CheckTimeout();
        bool Send();
        bool Receive();

        void OnMessageChoke();
        void OnMessageUnchoke();
        void OnMessageInterested();
        void OnMessageNotInterested();
        void OnMessageHave(std::uint32_t piece_index);
        void OnMessageBitfield(const ByteVector &bitfield);
        void OnMessageRequest(std::uint32_t index, std::uint32_t begin, std::uint32_t length);
        void OnMessagePiece(std::uint32_t index, std::uint32_t begin, const ByteVector &piece);
        void OnMessageCancel(std::uint32_t index, std::uint32_t begin, std::uint32_t length);
        void OnMessagePort(std::uint16_t port);

        PeerInfo peer_info_;
        std::shared_ptr<const Metainfo> metainfo_;
        PeerId own_peer_id_;

        unsigned nr_blocks_requested_;
        std::deque<PieceBlock> potential_blocks_to_request_; // Stores the blocks

        bool am_choking_;      // This client is choking the peer
        bool am_interested_;   // This client is interested in the peer
        bool peer_choking_;    // PeerConnection is choking this client
        bool peer_interested_; // PeerConnection is interested in this client
        bool can_receive_bitfield_;

        Status status_;
        std::chrono::steady_clock::time_point timeout_;
        network::Socket socket_;
        ByteVector send_buffer_;
        ByteVector receive_buffer_;
    };

}

#endif