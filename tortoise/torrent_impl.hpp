#ifndef TORTOISE_TORRENT_IMPL_HPP
#define TORTOISE_TORRENT_IMPL_HPP

#include <tortoise/torrent.hpp>

#include "event_queue.hpp"
#include "peer.hpp"
#include "peer_id.hpp"
#include "piece_manager.hpp"
#include "tracker/tracker_manager.hpp"

#include <list>

namespace tortoise
{
    class Torrent : public std::enable_shared_from_this<Torrent>
    {
    public:
        Torrent(const TorrentParameters &params, EventQueue &event_queue);
        ~Torrent();

        //! \brief Handles all of the torrent's logic. Should be called periodically.
        void Process();

        std::shared_ptr<const Metainfo> GetMetainfo() const;

    private:
        bool AddPeer(const PeerInfo &peer_info);
        void ProcessPeers();
        AnnounceParameters GetTrackerRequest();

        EventQueue &event_queue_;
        std::shared_ptr<Metainfo> metainfo_;
        const PeerId peer_id_;
        TrackerManager tracker_manager_;
        PieceManager piece_manager_;
        std::list<PeerInfo> peer_queue_;
        std::list<Peer> peers_;
    };
}

#endif