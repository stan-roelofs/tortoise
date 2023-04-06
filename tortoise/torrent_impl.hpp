#ifndef TORTOISE_TORRENT_IMPL_HPP
#define TORTOISE_TORRENT_IMPL_HPP

#include <tortoise/torrent.hpp>

#include "peer_id.hpp"
#include "tracker/tracker_manager.hpp"

namespace tortoise
{
    class Torrent
    {
    public:
        Torrent(const TorrentParameters &params);
        ~Torrent();

        //! \brief Handles all of the torrent's logic. Should be called periodically.
        void Process();

    private:
        AnnounceParameters GetTrackerRequest();
        TorrentParameters parameters_;
        const PeerId peer_id_;
        TrackerManager tracker_manager_;
    };
}

#endif