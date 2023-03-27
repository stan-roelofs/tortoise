#ifndef TORTOISE_UDP_TRACKER_CONNECTION_HPP
#define TORTOISE_UDP_TRACKER_CONNECTION_HPP

#include <tortoise/tracker_connection.hpp>

namespace tortoise
{
    //! \brief Implements the UDP tracker protocol as described in BEP 15.
    class UDPTrackerConnection : public TrackerConnection
    {
    public:
		/* \brief Creates a new UDP tracker connection.
		 *  \param url The URL of the tracker.
		 *  \throws UnsupportedProtocolException If the protocol is not supported.
		 */
        UDPTrackerConnection(const URL& url);
        
        bool Announce(const AnnounceParameters& parameters, std::function<void(Result, std::shared_ptr<AnnounceResponse> response)> result_callback, unsigned int timeout) override;

    private:
    };

} // namespace tortoise

#endif // TORTOISE_UDP_TRACKER_CONNECTION_HPP