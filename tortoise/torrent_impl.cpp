#include "torrent_impl.hpp"

#include <cassert>

#include "log.hpp"

namespace
{
    constexpr std::size_t DESIRED_PEERS = 20;
}

namespace tortoise
{
    Torrent::Torrent(const TorrentParameters &parameters) : metainfo_(std::make_shared<Metainfo>(parameters.metainfo)),
                                                            tracker_manager_(metainfo_->announce_list, std::bind(&Torrent::GetTrackerRequest, this)),
                                                            piece_manager_(metainfo_->pieces.size())
    {
        if (parameters.save_path.empty())
            throw TorrentException("save_path is empty"); // TODO set a default save path somewhere

        //
    }

    Torrent::~Torrent() = default;

    void Torrent::Process()
    {
        // TODO should each torrent have its own thread??
        if (tracker_manager_.Update())
        {
            const auto new_peers = tracker_manager_.GetPeers();
            for (const auto &peer : new_peers)
                AddPeer(peer);
        }

        ProcessPeers();
    }

    bool Torrent::AddPeer(const PeerInfo &peer_info)
    {
        if (std::find(peer_queue_.begin(), peer_queue_.end(), peer_info) != peer_queue_.end())
            return false;
        if (std::find_if(peers_.begin(), peers_.end(), [&peer_info](const auto &peer)
                         { return peer.GetPeerInfo() == peer_info; }) != peers_.end())
            return false;

        peer_queue_.push_back(peer_info);
        return true;
    }

    void Torrent::ProcessPeers()
    {
        // Add new peers if we don't have enough and there are still peers in the queue
        while (peers_.size() < DESIRED_PEERS && !peer_queue_.empty())
        {
            PeerInfo peer_info = peer_queue_.front();
            peer_queue_.pop_front();

            LOG("Torrent", "Adding peer %s %u", peer_info.ip.c_str(), peer_info.port);
            Peer::Callbacks callbacks;
            peers_.emplace_back(peer_info, metainfo_, peer_id_, callbacks);
        }

        auto it = peers_.begin();
        while (it != peers_.end())
        {
            if (!it->Process())
            {
                const auto &info = it->GetPeerInfo();
                (void)info;
                LOG("Torrent", "Peer %s %u finished", info.ip.c_str(), info.port);

                it = peers_.erase(it);
                continue;
            }

            ++it;
        }
    }

    AnnounceParameters Torrent::GetTrackerRequest()
    {
        AnnounceParameters request(metainfo_->info_hash, peer_id_);
        return request; // TODO
    }
}