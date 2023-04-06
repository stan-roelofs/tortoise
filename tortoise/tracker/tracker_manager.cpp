#include "tracker_manager.hpp"

#include <cassert>

#include <tortoise/exceptions.hpp>

#include "../http/exception.hpp"
#include "../http/request.hpp"
#include "../log.hpp"
#include "http_tracker_connection.hpp"
#include "udp_tracker_connection.hpp"

namespace
{
    /*! \brief Creates a tracker connection for the specified URL.
     *  \param url The URL of the tracker.
     *  \returns A tracker connection object.
     *  \throws UnsupportedProtocolException If the protocol is not supported.
     */
    std::unique_ptr<tortoise::TrackerConnection> CreateTrackerConnection(const tortoise::URL &url)
    {
        using namespace tortoise;

        if (url.GetProtocol() == "http")
            return std::make_unique<HTTPTrackerConnection>(url);
        else if (url.GetProtocol() == "udp")
            return std::make_unique<UDPTrackerConnection>(url);
        else
            throw UnsupportedProtocolException(url.GetProtocol());
    }
}

namespace tortoise
{
    TrackerManager::TrackerManager(const std::vector<std::vector<std::string>> &trackers, std::function<AnnounceParameters()> request_callback)
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
            return false;

        auto now = std::chrono::steady_clock::now();
        if (tracker_interval_seconds_ == 0 || (last_tracker_contact_ + std::chrono::seconds(tracker_interval_seconds_) < now))
            RequestTrackerUpdate();

        return false;
    }

    void TrackerManager::SelectNextTracker()
    {
        // TODO
        // assert(false);
    }

    void TrackerManager::RequestTrackerUpdate()
    {
        request_pending_ = true;

        AnnounceParameters parameters = request_callback_();
        parameters.compact = true;
        parameters.no_peer_id = true;
        parameters.event = AnnounceParameters::Event::Started;
        parameters.numwant = 10;

        assert(current_tracker_ != nullptr);
        try
        {
            request_ = CreateTrackerConnection(current_tracker_->url);

            request_->Announce(
                parameters, [this](TrackerConnection::Result result, std::shared_ptr<AnnounceResponse> response)
                {
                    if (result == TrackerConnection::Result::Success)
                    {
                        LOG("TrackerManager", "Request succeeded");
                        LOG("TrackerManager", "Peers:");
						for (const auto& peer : response->peers)
						{
							LOG("TrackerManager", "  %s:%d", peer.ip.c_str(), peer.port);
						}
                    }
                    else {
                        LOG("TrackerManager", "Request failed");
                    } },
                10000);
            LOG("TrackerManager", "Requesting tracker update from %s", current_tracker_->url.ToString().c_str());
        }
        catch (UnsupportedProtocolException &e)
        {
            LOG("TrackerManager", "Unsupported protocol: %s", e.what());

            request_pending_ = false;
            // TODO remove this tracker from the list
            SelectNextTracker();
            return;
        }
    }

    void TrackerManager::OnTrackerResponse(const AnnounceResponse &)
    {
        // request_pending_ = false;

        // if (response.Failure())
        // {
        //     SelectNextTracker();
        //     return;
        // }

        // last_tracker_contact_ = std::chrono::steady_clock::now();
        // tracker_interval_seconds_ = response.min_interval ? *response.min_interval : response.interval;
    }
}