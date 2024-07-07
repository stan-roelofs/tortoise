#include "tracker_announce.hpp"

namespace
{
    constexpr std::uint16_t DEFAULT_BITTORRENT_PORT = 6881;
}

namespace tortoise
{
    namespace tracker
    {
        AnnounceParameters::AnnounceParameters(const SHA1Hash &infohash, const PeerId &peerid)
            : info_hash(infohash),
              peer_id(peerid),
              port(DEFAULT_BITTORRENT_PORT),
              downloaded(0),
              uploaded(0),
              left(0),
              compact(true),
              no_peer_id(false),
              event(Event::None)
        {
        }

        AnnounceResponse::AnnounceResponse()
            : interval(0),
              complete(0),
              incomplete(0)
        {
        }
    }
}