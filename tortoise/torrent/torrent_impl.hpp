#ifndef TORTOISE_TORRENT_IMPL_HPP
#define TORTOISE_TORRENT_IMPL_HPP

#include <tortoise/torrent.hpp>

#include "connected_peer.hpp"
#include "event_queue.hpp"
#include "peer_connection.hpp"
#include "peer_id.hpp"
#include "piece_manager.hpp"

#include <atomic>
#include <list>
#include <map>
#include <thread>

namespace tortoise
{
    class Torrent
    {
    public:
        class PeerInfoProvider
        {
        public:
            virtual ~PeerInfoProvider() = default;
            virtual void RegisterTorrent(const Torrent &torrent, std::function<void(const std::vector<PeerInfo> &)> callback) = 0;
            virtual void UnregisterTorrent(const Torrent &torrent) = 0;
            virtual void RequestPeers(const Torrent &torrent, unsigned desired) = 0;
        };

        Torrent(const TorrentParameters &params, PeerInfoProvider &peer_info_provider, EventQueue &event_queue);
        ~Torrent();

        void Start();
        void Stop();

        std::shared_ptr<const Metainfo> GetMetainfo() const;
        PeerId GetPeerId() const;

    private:
        void OnNewPeers(const std::vector<PeerInfo> &new_peers);

        static void Run(Torrent &torrent);
        void ProcessPeers();

        std::atomic_bool running_;
        mutable std::recursive_mutex mutex_;
        std::thread thread_;

        bool requested_peers_;
        PeerInfoProvider &peer_info_provider_;
        EventQueue &event_queue_;
        std::shared_ptr<const Metainfo> metainfo_;
        const PeerId peer_id_;
        PieceManager piece_manager_;

        std::list<PeerInfo> potential_peers_;
        std::list<std::unique_ptr<PeerConnection>> pending_peers_;
        std::list<std::unique_ptr<ConnectedPeer>> connected_peers_;
    };
}

#endif