#include "tracker_manager.hpp"

#include <cassert>

#include <tortoise/exceptions.hpp>

#include "http_tracker_connection.hpp"
#include "log.hpp"
#include "udp_tracker_connection.hpp"

namespace
{
    /*! \brief Creates a tracker connection for the specified URL.
     *  \param protocol The protocol to use, e.g. http, https, udp.
     *  \returns A tracker connection object.
     *  \throws UnsupportedProtocolException If the protocol is not supported.
     */
    std::unique_ptr<tortoise::TrackerConnection> CreateTrackerConnection(const std::string &protocol)
    {
        using namespace tortoise;

        if (protocol == "http")
            return std::make_unique<HTTPTrackerConnection>();
        else if (protocol == "udp")
            return std::make_unique<UDPTrackerConnection>();
        else
            throw UnsupportedProtocolException(protocol);
    }
}

namespace tortoise
{
    TrackerManager::TrackerManager(const std::vector<std::vector<std::string>> &trackers, std::function<AnnounceParameters()> request_callback)
        : tracker_interval_seconds_(0),
          request_callback_(request_callback),
          tracker_list_(trackers)
    {
    }

    TrackerManager::~TrackerManager() = default;

    bool TrackerManager::Update()
    {
        if (request_)
        {
            if (!request_->Process())
                return false; // Request not finished yet

            TrackerConnection::Result result = request_->GetLastResult();
            request_.reset();

            if (HandleTrackerResult(result))
            {
                tracker_list_.PromoteCurrentTracker();
                tracker_list_.SelectFirstTracker();
                return true; // Request done
            }

            // If the request failed, try the next tracker
            SelectNextTracker();
        }

        auto now = std::chrono::steady_clock::now();
        if (tracker_interval_seconds_ == 0 || (last_tracker_contact_ + std::chrono::seconds(tracker_interval_seconds_) < now))
            RequestTrackerUpdate();

        return false;
    }

    void TrackerManager::SelectNextTracker()
    {
        tracker_interval_seconds_ = 0;
        last_tracker_contact_ = {};
        tracker_list_.SelectNextTracker();
    }

    void TrackerManager::RequestTrackerUpdate()
    {
        AnnounceParameters parameters = request_callback_();
        parameters.compact = true;
        parameters.no_peer_id = true;
        parameters.event = AnnounceParameters::Event::Started;
        parameters.numwant = 10;

        const URL current_tracker{tracker_list_.GetCurrentTracker()};
        try
        {
            LOG("TrackerManager", "Requesting tracker update from %s", current_tracker.ToString().c_str());

            request_ = CreateTrackerConnection(current_tracker.GetProtocol());
            const bool started = request_->Announce(current_tracker, parameters);
            assert(started);
            // TODO if not started??
        }
        catch (UnsupportedProtocolException &e)
        {
            LOG("TrackerManager", "Unsupported protocol: %s", e.what());
            // TODO remove this tracker from the list we can never use it
            SelectNextTracker();
            return;
        }
    }

    bool TrackerManager::HandleTrackerResult(const TrackerConnection::Result &result)
    {
        if (!result.success)
            return false;

        assert(result.response.has_value());

        LOG("TrackerManager", "Request succeeded");
        LOG("TrackerManager", "Peers:");
        for (const auto &peer : result.response->peers)
            LOG("TrackerManager", "  %s:%d", peer.ip.c_str(), peer.port);

        peers_ = result.response->peers;

        tracker_interval_seconds_ = result.response->min_interval ? result.response->min_interval.value() : result.response->interval;

        if (result.response->tracker_id.has_value())
        {
            // TODO
        }

        LOG("TrackerManager", "Tracker interval: %llu seconds", tracker_interval_seconds_);

        last_tracker_contact_ = std::chrono::steady_clock::now();
        return true;
    }

    std::list<PeerInfo> TrackerManager::GetPeers() const
    {
        return peers_;
    }
}