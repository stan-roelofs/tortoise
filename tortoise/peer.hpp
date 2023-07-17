#ifndef TORTOISE_PEER_HPP
#define TORTOISE_PEER_HPP

#include <chrono>

#include "peer_info.hpp"
#include "socket.hpp"

namespace tortoise
{
    class Peer
    {
    public:
        Peer(const PeerInfo &peer_info);
        ~Peer();

        void Process();

        //! \returns True if this peer can be removed from the torrent.
        bool Finished() const;

    private:
        PeerInfo peer_info_;

        Socket socket_;

        bool am_choking;      // This client is choking the peer
        bool am_interested;   // This client is interested in the peer
        bool peer_choking;    // Peer is choking this client
        bool peer_interested; // Peer is interested in this client

        std::chrono::steady_clock::time_point last_message; // The last time a message was received from this peer

        enum class State
        {
            Connect,
            Handshake,
            Finished
        };
        State state_;
    };
}

#endif