#ifndef TORTOISE_TRACKER_MANAGER_HPP
#define TORTOISE_TRACKER_MANAGER_HPP

#include <chrono>
#include <functional>

#include "http/request.hpp"
#include "sha1_hash.hpp"
#include "tracker.hpp"
#include "url.hpp"

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
        TrackerManager(const std::vector<std::vector<URL>> &trackers, std::function<TrackerRequest()> request_callback);
        ~TrackerManager();

        /*! \brief Updates the tracker information.
         *  \returns True if new data was received.
         */
        bool Update();

    private:
        void RequestTrackerUpdate();
        void OnTrackerResponse(const TrackerResponse &response);
        void SelectNextTracker();
        bool Receive();

        std::chrono::steady_clock::time_point last_tracker_contact_;
        std::uint64_t tracker_interval_seconds_;
        bool request_pending_;
        std::function<TrackerRequest()> request_callback_;

        struct Tracker
        {
            URL url;
            std::string tracker_id;
        };
        std::vector<std::list<Tracker>> trackers_;
        Tracker *current_tracker_;
        std::unique_ptr<http::AsyncRequest> request_;
        std::chrono::steady_clock::time_point request_start_time_;
    };
}

#endif