#ifndef TORTOISE_TRACKER_HPP
#define TORTOISE_TRACKER_HPP

#include <string>

#include "connection.hpp"
#include "metainfo.hpp"

namespace tortoise
{
    class Tracker
    {
    public:
        Tracker(const Metainfo &info);

        //! \brief Sends an announce request to the tracker and blocks until a response is received.
        bool SendAnnounce();

    private:
        Metainfo info_;
    };
} // namespace tortoise

#endif