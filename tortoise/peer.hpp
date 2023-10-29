#ifndef TORTOISE_PEER_HPP
#define TORTOISE_PEER_HPP

#include <chrono>
#include <functional>
#include <memory>
#include <set>

#include <tortoise/metainfo.hpp>

#include "peer_info.hpp"
#include "socket.hpp"
#include "util.hpp"

namespace tortoise
{
    class Peer
    {
    public:
        struct Callbacks
        {
            std::function<void(Peer &)> on_connect;                                                                  // Called when the peer connects.
            std::function<void(Peer &)> on_choke;                                                                    // Called when the peer chokes this client.
            std::function<void(Peer &)> on_unchoke;                                                                  // Called when the peer unchokes this client.
            std::function<void(Peer &)> on_interested;                                                               // Called when the peer is interested in this client.
            std::function<void(Peer &)> on_not_interested;                                                           // Called when the peer is not interested in this client.
            std::function<void(Peer &, std::set<std::uint32_t> pieces)> on_new_have;                                 // Called when the peer tells
            std::function<void(Peer &, std::uint32_t index, std::uint32_t begin, std::uint32_t length)> on_request;  // Called when the peer requests a piece.
            std::function<void(Peer &, std::uint32_t index, std::uint32_t begin, const ByteVector &piece)> on_piece; // Called when the peer sends a piece.
            std::function<void(Peer &, std::uint32_t index, std::uint32_t begin, std::uint32_t length)> on_cancel;   // Called when the peer cancels a request.
            std::function<void(Peer &, std::uint16_t port)> on_port;                                                 // Called when the peer sends a port message.
        };

        /*!
         * \brief Creates a new peer.
         * \param peer_info The peer info.
         * \param info_hash The info hash of the torrent, used for the handshake.
         * \param peer_id The peer id of this client, used for the handshake.
         * \param callbacks The callbacks to be called on certain events.
         */
        Peer(const PeerInfo &peer_info, std::shared_ptr<Metainfo> metainfo, PeerId peer_id, Callbacks callbacks);
        ~Peer();

        /*! \brief Handles all of the peer's logic. Should be called periodically.
         *  \return True if the peer is trying to connect or connected, false otherwise.
         */
        bool Process();

        const PeerInfo &GetPeerInfo() const;

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
        std::shared_ptr<Metainfo> metainfo_;
        PeerId own_peer_id_;
        Callbacks callbacks_;

        bool am_choking_;      // This client is choking the peer
        bool am_interested_;   // This client is interested in the peer
        bool peer_choking_;    // PeerConnection is choking this client
        bool peer_interested_; // PeerConnection is interested in this client
        std::set<std::uint32_t> has_pieces_;
        bool can_receive_bitfield_;

        enum class State
        {
            Connecting,
            Connected,
            Handshaking,
            Finished
        };

        State state_;
        std::chrono::steady_clock::time_point timeout_;
        Socket socket_;
        ByteVector send_buffer_;
        ByteVector receive_buffer_;
    };

}

#endif