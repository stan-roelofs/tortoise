#include <tortoise/tracker_announce.hpp>

namespace
{
    constexpr std::uint16_t DEFAULT_BITTORRENT_PORT = 6881;
}

namespace tortoise
{
    AnnounceParameters::AnnounceParameters(const SHA1Hash &infohash, const PeerId &peerid)
        : info_hash(infohash),
          peer_id(peerid),
          port(DEFAULT_BITTORRENT_PORT),
          downloaded(0),
          uploaded(0),
          left(0)
    {
    }

    AnnounceResponse::AnnounceResponse()
        : interval(0),
          min_interval(0),
          complete(0),
          incomplete(0)
    {
    }
}