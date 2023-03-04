#include <tortoise/tracker_manager.hpp>

#include <cassert>

#include <tortoise/exceptions.hpp>
#include <tortoise/http/exception.hpp>
#include <tortoise/http/request.hpp>

#include "log.hpp"

namespace
{
    constexpr int TIMEOUT_SECONDS = 10;
}

namespace tortoise
{
    // TODO we could just have a single instance on its own thread and have it manage all torrents
    TrackerManager::TrackerManager(const std::vector<std::vector<URL>> &trackers, std::function<TrackerRequest()> request_callback)
        : tracker_interval_seconds_(0),
          request_pending_(false),
          request_callback_(request_callback),
          current_tracker_(nullptr)
    {
        if (trackers.empty() || trackers[0].empty())
            throw InvalidArgumentException("trackers list is empty");

        for (const auto &tier : trackers)
        {
            std::list<Tracker> tier_trackers;
            for (const auto &url : tier)
                tier_trackers.push_back({url, ""});
            trackers_.push_back(tier_trackers);
        }

        current_tracker_ = &trackers_.front().front();
    }

    TrackerManager::~TrackerManager() = default;

    bool TrackerManager::Update()
    {
        if (request_pending_)
            return Receive();

        auto now = std::chrono::steady_clock::now();
        if (tracker_interval_seconds_ == 0 || (last_tracker_contact_ + std::chrono::seconds(tracker_interval_seconds_) < now))
            RequestTrackerUpdate();

        return false;
    }

    bool TrackerManager::Receive()
    {
        const bool done = request_->Process();
        if (!done)
        {
            if (std::chrono::steady_clock::now() - request_start_time_ > std::chrono::seconds(TIMEOUT_SECONDS))
            {
                LOG("TrackerManager", "Request timed out");
                request_.reset();
                request_pending_ = false;
            }
            return false;
        }

        if (!request_->GetResponse() || !request_->GetResponse()->Success())
        {
            LOG("TrackerManager", "Request failed");
            SelectNextTracker();
            request_.reset();
            request_pending_ = false;
            return false;
        }

        LOG("TrackerManager", "Request succeeded");
        // TrackerResponse response(request_->GetResponse()->GetBody());
        //  TODO

        request_.reset();
        request_pending_ = false;
        return true;
    }

    void TrackerManager::SelectNextTracker()
    {
        // TODO
        assert(false);
    }

    void TrackerManager::RequestTrackerUpdate()
    {
        request_pending_ = true;

        TrackerRequest request = request_callback_();
        request.compact = true;
        request.no_peer_id = true;

        assert(current_tracker_ != nullptr);
        try
        {
            request_ = std::make_unique<http::AsyncRequest>(current_tracker_->url, request.GetParameters());
            LOG("TrackerManager", "Requesting tracker update from %s", current_tracker_->url.ToString().c_str());
            request_->Process();
            request_start_time_ = std::chrono::steady_clock::now();
        }
        catch (http::Exception &)
        {
            request_pending_ = false;
            // TODO remove this tracker from the list
            SelectNextTracker();
            return;
        }
    }

    void TrackerManager::OnTrackerResponse(const TrackerResponse &response)
    {
        request_pending_ = false;

        if (response.Failure())
        {
            SelectNextTracker();
            return;
        }

        last_tracker_contact_ = std::chrono::steady_clock::now();
        tracker_interval_seconds_ = response.min_interval ? *response.min_interval : response.interval;
    }
}