#ifndef TORTOISE_PEER_HPP
#define TORTOISE_PEER_HPP

#include <chrono>

#include <tortoise/sha1_hash.hpp>

#include "peer_info.hpp"
#include "socket.hpp"
#include "util.hpp"

namespace tortoise
{
    //! \brief Manages a connection to a peer in a torrent.
    class Peer
    {
    public:
        /*!
         * \brief Creates a new peer.
         * \param peer_info The peer info.
         * \param info_hash The info hash of the torrent, used for the handshake.
         * \param peer_id The peer id of this client, used for the handshake.
         */
        Peer(PeerInfo peer_info, SHA1Hash info_hash, PeerId peer_id);
        ~Peer();

        /*!
         * \brief Processes the peer.
         * \returns Whether the peer is still active, i.e. false indicates the peer has finished.
         */
        bool Process();

        const PeerInfo &GetInfo() const;

    private:
        /*! \brief Sends data from the send buffer to the peer.
         * \returns Whether sending was successful.
         */
        bool Send();

        /*! \brief Receives data from the peer into the receive buffer.
         * \returns Whether receiving was successful.
         */
        bool Receive();

        void Error(const std::string &reason);
        void ShiftBuffer(std::size_t amount);

        // Connection management
        void ResetTimeout();
        //! \returns Whether the peer has timed out.
        bool CheckTimeout();
        //! \returns Whether the connection is active.
        bool Connect();

        // Message handlers
        bool HandleMessages();
        void OnChoke();
        void OnUnchoke();
        void OnInterested();
        void OnNotInterested();
        void OnHave(std::uint32_t piece_index);
        void OnBitfield(const ByteVector &bitfield);
        void OnRequest(std::uint32_t piece_index, std::uint32_t begin, std::uint32_t length);
        void OnPiece(std::uint32_t piece_index, std::uint32_t begin, const ByteVector &block);
        void OnCancel(std::uint32_t piece_index, std::uint32_t begin, std::uint32_t length);
        void OnPort(std::uint16_t port);

        // Information about the peer and torrent
        PeerInfo peer_info_;
        SHA1Hash info_hash_;
        PeerId peer_id_;

        // Peer state
        bool am_choking_;      // This client is choking the peer
        bool am_interested_;   // This client is interested in the peer
        bool peer_choking_;    // Peer is choking this client
        bool peer_interested_; // Peer is interested in this client

        // Connection variables
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