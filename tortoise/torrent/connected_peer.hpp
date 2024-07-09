#ifndef TORTOISE_PEER_HPP
#define TORTOISE_PEER_HPP

#include "peer_connection.hpp"
#include "peer_info.hpp"

namespace tortoise
{
    class ConnectedPeer
    {
    public:
        ConnectedPeer(std::unique_ptr<PeerConnection> connection);
        ~ConnectedPeer();

        const PeerInfo &GetPeerInfo() const;
        PeerConnection::Status GetConnectionStatus() const;

        void Process();

    private:
        std::unique_ptr<PeerConnection> connection_;
    };
}

#endif