#ifndef TORTOISE_TRACKER_HPP
#define TORTOISE_TRACKER_HPP

#include <string>

#include "metainfo.hpp"

namespace tortoise
{
    class Tracker
    {
    public:
        Tracker(const Metainfo &info);

        /*! \brief Sends an announce to the tracker and blocks until a response is received.
        *   \throws TrackerException If communication with the tracker failed. 
        */
        void Announce();

        struct PeerInfo
        {
			std::string ip;
			std::uint16_t port;
        };

    private:
        bool HandleResponse(const std::string& response);
        
        Metainfo info_;
    };
} // namespace tortoise

#endif