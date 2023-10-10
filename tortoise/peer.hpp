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
        /**
         * \brief Creates a new peer.
         * \param peer_info The peer info.
         * \param info_hash The info hash of the torrent, used for the handshake.
         * \param peer_id The peer id of this client, used for the handshake.
         */
        Peer(PeerInfo peer_info, SHA1Hash info_hash, PeerId peer_id);
        ~Peer();

        void Process();

        //! \returns True if this peer can be removed from the torrent.
        bool Finished() const;

        const PeerInfo &GetInfo() const;

    private:
        void CheckTimeout();
        void Send();
        void Receive(int length);
        void ResetTimeout();

        PeerInfo peer_info_;
        SHA1Hash info_hash_;
        PeerId peer_id_;

        Socket socket_;

        bool am_choking_;      // This client is choking the peer
        bool am_interested_;   // This client is interested in the peer
        bool peer_choking_;    // Peer is choking this client
        bool peer_interested_; // Peer is interested in this client

        std::chrono::steady_clock::time_point timeout_; // Do we need seperate send/receive timeouts?

        ByteVector send_buffer_;
        ByteVector receive_buffer_;

        enum class State
        {
            Connect,
            Handshake_Send,
            Handshake_Receive,
            Handshake_Done,
            Finished
        };
        State state_;
    };
}

#endif