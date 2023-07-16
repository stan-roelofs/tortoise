#ifndef TORTOISE_TRACKER_MANAGER_HPP
#define TORTOISE_TRACKER_MANAGER_HPP

#include <chrono>
#include <functional>
#include <list>

#include <tortoise/sha1_hash.hpp>

#include "tracker_announce.hpp"
#include "tracker_connection.hpp"
#include "tracker_list.hpp"
#include "../url.hpp"

namespace tortoise
{
    //! \brief Manages the tracker requests and responses. Implements the multi-tracker logic as described in BEP 12.
    class TrackerManager
    {
    public:
        /*! \param trackers The list of trackers to use.
         *  \param request_callback A callback that returns the request to send to the tracker.
         *  \throws InvalidArgumentException If the trackers list is empty.
         */
        TrackerManager(const std::vector<std::vector<std::string>> &trackers, std::function<AnnounceParameters()> request_callback);
        ~TrackerManager();

        /*! \brief Updates the tracker information.
         *  \returns True if new data was received.
         */
        bool Update();

    private:
        void RequestTrackerUpdate();
        void SelectNextTracker();
        bool HandleTrackerResult(const TrackerConnection::Result &result);

        std::chrono::steady_clock::time_point last_tracker_contact_;
        std::uint64_t tracker_interval_seconds_;
        std::function<AnnounceParameters()> request_callback_;

        TrackerList tracker_list_;
        std::unique_ptr<TrackerConnection> request_;
    };
}

#endif