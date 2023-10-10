#include "torrent_impl.hpp"

#include <cassert>

#include "log.hpp"

namespace
{
    constexpr std::size_t DESIRED_PEERS = 20;
}

namespace tortoise
{
    Torrent::Torrent(const TorrentParameters &parameters) : parameters_(parameters),
                                                            tracker_manager_(parameters_.metainfo.announce_list, std::bind(&Torrent::GetTrackerRequest, this)),
                                                            piece_manager_(parameters_.metainfo.pieces.size())
    {
        if (parameters.save_path.empty())
            throw TorrentException("save_path is empty"); // TODO set a default save path somewhere

        //
    }

    Torrent::~Torrent() = default;

    void Torrent::Process()
    {
        if (tracker_manager_.Update())
        {
            const auto new_peers = tracker_manager_.GetPeers();
            for (const auto &peer : new_peers)
                peer_queue_.push_front(peer);
        }

        ProcessPeers();
    }

    void Torrent::ProcessPeers()
    {
        // Add new peers if we don't have enough and there are still peers in the queue
        while (peers_.size() < DESIRED_PEERS && !peer_queue_.empty())
        {
            PeerInfo peer_info = peer_queue_.front();
            peer_queue_.pop_front();

            LOG("Torrent", "Adding peer %s %u", peer_info.ip.c_str(), peer_info.port);
			peers_.emplace_back(peer_info, parameters_.metainfo.info_hash, peer_id_);
        }

        auto it = peers_.begin();
        while (it != peers_.end())
        {
            if (it->Finished())
            {
                const auto &info = it->GetInfo();
                LOG("Torrent", "Peer %s %u finished", info.ip.c_str(), info.port);

                it = peers_.erase(it);
                continue;
            }

            it->Process();
            ++it;
        }
    }

    AnnounceParameters Torrent::GetTrackerRequest()
    {
        AnnounceParameters request(parameters_.metainfo.info_hash, peer_id_);
        return request; // TODO
    }
}