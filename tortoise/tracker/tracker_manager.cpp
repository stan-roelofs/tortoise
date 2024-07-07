#include "tracker_manager.hpp"

#include <cassert>
#include <future>

#include <tortoise/exceptions.hpp>

#include "log.hpp"
#include "tracker_connection.hpp"


namespace tortoise
{
    namespace
    {
        std::future<std::optional<AnnounceResponse>> Announce(URL url, std::shared_ptr<const AnnounceParameters> parameters, std::shared_ptr<std::atomic_bool> cancel)
        {
            assert(parameters);
            if (!parameters)
                return {};

            if (url.GetProtocol() == "http")
                return std::async(std::launch::async, [=]()
                                  { return tracker::http::Announce(url, parameters, cancel); });
            else if (url.GetProtocol() == "udp")
                return std::async(std::launch::async, [=]()
                                  { return tracker::udp::Announce(url, parameters, cancel); });

            throw UnsupportedProtocolException(url.GetProtocol());
        }
    }

    TrackerManager::TrackerManager() : thread_(TrackerManager::Run, std::ref(*this)), running_(true)
    {
    }

    TrackerManager::~TrackerManager()
    {
        std::lock_guard guard(mutex_);
        running_ = false;

        torrents_.clear();

        if (thread_.joinable())
            thread_.join();
    }

    void TrackerManager::Run(TrackerManager &tracker_manager)
    {
        while (true)
        {
            {
                std::lock_guard guard(tracker_manager.mutex_);
                // TODO: could sort the torrents by the time until the next request, then stop when we find the first one that does not need a request
                for (auto &torrent : tracker_manager.torrents_)
                    torrent->Process();
                if (!tracker_manager.running_)
                    break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    TrackerManager::TorrentTrackerData::TorrentTrackerData(const Torrent &torrent, std::function<void(const PeerInfo &)> callback)
        : torrent(&torrent),
          callback_(std::move(callback)),
          tracker_list_(torrent.GetMetainfo()->announce_list),
          tracker_interval_seconds_(0),
          cancel_flag_(std::make_shared<std::atomic_bool>(false))
    {
    }

    TrackerManager::TorrentTrackerData::~TorrentTrackerData()
    {
        if (request_)
            *cancel_flag_ = true;

        request_->wait();
    }

    void TrackerManager::TorrentTrackerData::Process()
    {
        auto now = std::chrono::steady_clock::now();

        if (request_.has_value())
        {
            if (!request_->valid())
            {
                if (now > timeout_)
                {
                    *cancel_flag_ = true;
                    LOG("TrackerManager", "Timed out waiting for tracker %s", tracker_list_.GetCurrentTracker().c_str());
                    request_ = std::nullopt;
                    SelectNextTracker();
                }
                return;
            }

            if (HandleTrackerResult(request_->get()))
            {
                tracker_list_.PromoteCurrentTracker();
                tracker_list_.SelectFirstTracker();
            }
            else
                SelectNextTracker();

            request_ = std::nullopt;
        }

        if (tracker_interval_seconds_ == 0 || (last_tracker_contact_ + std::chrono::seconds(tracker_interval_seconds_) < now))
            RequestTrackerUpdate();
    }

    void TrackerManager::TorrentTrackerData::Cancel()
    {
        *cancel_flag_ = true;
    }

    void TrackerManager::TorrentTrackerData::SelectNextTracker()
    {
        tracker_interval_seconds_ = 0;
        last_tracker_contact_ = {};
        tracker_list_.SelectNextTracker();
    }

    void TrackerManager::TorrentTrackerData::RequestTrackerUpdate()
    {
        std::shared_ptr<AnnounceParameters> parameters = std::make_shared<AnnounceParameters>(torrent->GetMetainfo()->info_hash, torrent->GetPeerId());
        parameters->compact = true;
        parameters->no_peer_id = true;
        parameters->event = AnnounceParameters::Event::Started;
        parameters->numwant = 10;

        const URL current_tracker{tracker_list_.GetCurrentTracker()};
        try
        {
            LOG("TrackerManager", "Requesting tracker update from %s", current_tracker.ToString().c_str());
            request_ = Announce(current_tracker, parameters, cancel_flag_);
            timeout_ = std::chrono::steady_clock::now() + std::chrono::seconds(60);
        }
        catch (UnsupportedProtocolException &e)
        {
            LOG("TrackerManager", "Unsupported protocol: %s", e.what());
            tracker_list_.RemoveCurrentTracker();
            SelectNextTracker();
            return;
        }
    }

    bool TrackerManager::TorrentTrackerData::HandleTrackerResult(const std::optional<AnnounceResponse> &result)
    {
        if (!result)
        {
            LOG("TrackerManager", "Request failed");
            return false;
        }

        if (result->failure_reason)
        {
            LOG("TrackerManager", "Request failed: %s", result->failure_reason->c_str());
            return false;
        }

        LOG("TrackerManager", "Request succeeded");
        LOG("TrackerManager", "Peers:");
        for (const auto &peer : result->peers)
        {
            (void)peer;
            LOG("TrackerManager", "  %s:%d", peer.ip.c_str(), peer.port);
            callback_(peer);
        }

        tracker_interval_seconds_ = result->min_interval ? result->min_interval.value() : result->interval;

        if (result->tracker_id.has_value())
        {
            // TODO
        }

        last_tracker_contact_ = std::chrono::steady_clock::now();
        return true;
    }

    void TrackerManager::RequestPeers(const Torrent &torrent, std::function<void(const PeerInfo &)> callback)
    {
        // Remove if the torrent is already in the list
        CancelRequest(torrent);

        std::lock_guard lock(mutex_);
        torrents_.push_back(std::make_unique<TorrentTrackerData>(torrent, callback));
    }

    void TrackerManager::CancelRequest(const Torrent &torrent)
    {
        std::lock_guard lock(mutex_);

        const auto it = std::find_if(torrents_.begin(), torrents_.end(), [&torrent](const std::unique_ptr<TorrentTrackerData> &data)
                                     { return data->torrent == &torrent; });

        if (it != torrents_.end())
            torrents_.erase(it);
    }
}