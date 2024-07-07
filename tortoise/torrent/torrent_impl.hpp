#ifndef TORTOISE_TORRENT_IMPL_HPP
#define TORTOISE_TORRENT_IMPL_HPP

#include <tortoise/torrent.hpp>

#include "event_queue.hpp"
#include "peer_connection.hpp"
#include "peer_id.hpp"
#include "piece_manager.hpp"

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
            virtual void RegisterTorrent(const Torrent &torrent, std::function<void(const PeerInfo &)> callback) = 0;
            virtual void UnregisterTorrent(const Torrent &torrent) = 0;
            virtual void RequestPeers(const Torrent &torrent) = 0;
        };

        Torrent(const TorrentParameters &params, PeerInfoProvider &peer_info_provider, EventQueue& event_queue);
        ~Torrent();

        void Start();
        void Stop();

        std::shared_ptr<const Metainfo> GetMetainfo() const;
        PeerId GetPeerId() const;

    private:
        void AddPeer(const PeerInfo &peer_info);

        static void Run(Torrent &torrent);
        void ProcessPeers();

        bool running_;
        mutable std::recursive_mutex mutex_;
        std::thread thread_;

        PeerInfoProvider &peer_info_provider_;
        EventQueue& event_queue_;
        std::shared_ptr<const Metainfo> metainfo_;
        const PeerId peer_id_;
        PieceManager piece_manager_;

        std::list<PeerInfo> potential_peers_;
        std::list<std::shared_ptr<PeerConnection>> current_peers_;
    };
}

#endif