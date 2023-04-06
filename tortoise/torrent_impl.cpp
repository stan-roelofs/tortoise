#include "torrent_impl.hpp"

#include <cassert>

namespace tortoise
{
    Torrent::Torrent(const TorrentParameters &parameters) : parameters_(parameters), tracker_manager_(parameters_.metainfo.announce_list, std::bind(&Torrent::GetTrackerRequest, this))
    {
        if (parameters.save_path.empty())
            throw TorrentException("save_path is empty"); // TODO set a default save path somewhere
    }

    Torrent::~Torrent() = default; // TODO

    void Torrent::Process()
    {
        if (tracker_manager_.Update())
        {
            // TODO we got new data from the tracker
        }
    }

    AnnounceParameters Torrent::GetTrackerRequest()
    {
        AnnounceParameters request(parameters_.metainfo.info_hash, peer_id_);
        return request; // TODO
    }
}