#ifndef TORTOISE_TRACKER_CONNECTION_HPP
#define TORTOISE_TRACKER_CONNECTION_HPP

#include <future>

#include "tracker_announce.hpp"

#include <atomic>
#include <memory>

#include "tracker_announce.hpp"

namespace tortoise
{
    namespace tracker
    {
        namespace http
        {
            std::optional<AnnounceResponse> Announce(URL url, std::shared_ptr<const AnnounceParameters> parameters, std::shared_ptr<std::atomic_bool> cancel);
        }
        namespace udp
        {
            std::optional<AnnounceResponse> Announce(URL url, std::shared_ptr<const AnnounceParameters> parameters, std::shared_ptr<std::atomic_bool> cancel);
        }
    }
}

#endif // TORTOISE_TRACKER_CONNECTION_HPP