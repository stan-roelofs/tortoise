#include "connected_peer.hpp"

namespace tortoise
{
    ConnectedPeer::ConnectedPeer(std::unique_ptr<PeerConnection> connection) : connection_(std::move(connection)){};
    ConnectedPeer::~ConnectedPeer() = default;

    const PeerInfo &ConnectedPeer::GetPeerInfo() const
    {
        return connection_->GetPeerInfo();
    }

    PeerConnection::Status ConnectedPeer::GetConnectionStatus() const
    {
        return connection_->GetStatus();
    }

    void ConnectedPeer::Process()
    {
        connection_->Process();
    }
}