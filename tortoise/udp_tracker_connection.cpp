#include "udp_tracker_connection.hpp"

namespace tortoise
{
    UDPTrackerConnection::UDPTrackerConnection(const URL &url) : TrackerConnection(url)
    {
        if (url.GetProtocol() != "udp")
            throw UnsupportedProtocolException(url.GetProtocol());
    }

    bool UDPTrackerConnection::Announce(const AnnounceParameters &parameters, std::function<void(Result, std::shared_ptr<AnnounceResponse> response)> result_callback, unsigned int timeout)
    {
		// 1. Connect to the tracker.

        (void)parameters;
        (void)result_callback;
        (void)timeout;
        throw UnsupportedProtocolException("Not implemented"); // TODO
    }

}