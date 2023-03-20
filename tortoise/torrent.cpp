#include <tortoise/torrent.hpp>

#include <cassert>

namespace tortoise
{
    Torrent::Torrent(const Parameters& parameters) : 
        parameters_(parameters),
        tracker_manager_(parameters_.metainfo.GetAnnounceList(), std::bind(&Torrent::GetTrackerRequest, this))
    {
        if (parameters.save_path.empty())
            throw TorrentException("save_path is empty"); // TODO set a default save path somewhere
    }

    Torrent::~Torrent()
    {
    }

    void Torrent::Process()
    {
        if (tracker_manager_.Update())
        {
			// TODO we got new data from the tracker
        }
    }

    AnnounceParameters Torrent::GetTrackerRequest()
    {
        AnnounceParameters request(parameters_.metainfo.GetInfoHash(), parameters_.peer_id);
        return request; // TODO
    }
}