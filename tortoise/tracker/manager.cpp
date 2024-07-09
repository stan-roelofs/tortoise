#include "manager.hpp"

#include <cassert>
#include <future>

#include <tortoise/exceptions.hpp>

#include "../util/log.hpp"
#include "connection.hpp"

namespace tortoise
{
    namespace tracker
    {
        namespace
        {
            std::future<std::optional<AnnounceResponse>> Announce(network::URL url, std::shared_ptr<const AnnounceParameters> parameters, std::shared_ptr<std::atomic_bool> cancel)
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

        Manager::Manager() : thread_(Manager::Run, std::ref(*this)), running_(true)
        {
        }

        Manager::~Manager()
        {
            std::lock_guard guard(mutex_);
            running_ = false;

            torrents_.clear();

            if (thread_.joinable())
                thread_.join();
        }

        void Manager::Run(Manager &tracker_manager)
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

        Manager::TorrentTrackerData::TorrentTrackerData(const Torrent &torrent, std::function<void(const std::vector<PeerInfo> &)> callback)
            : torrent(&torrent),
              callback(std::move(callback)),
              tracker_list_(torrent.GetMetainfo()->announce_list),
              request_parameters_(std::make_shared<AnnounceParameters>(torrent.GetMetainfo()->info_hash, torrent.GetPeerId())),
              tracker_interval_seconds_(0),
              cancel_flag_(std::make_shared<std::atomic_bool>(false))
        {
            request_parameters_->compact = true;
            request_parameters_->no_peer_id = true;
            request_parameters_->event = AnnounceParameters::Event::Started;
            request_parameters_->numwant = 10;
        }

        Manager::TorrentTrackerData::~TorrentTrackerData()
        {
            std::lock_guard lock(mutex_);
            if (request_)
            {
                *cancel_flag_ = true;
                request_->wait();
            }
        }

        void Manager::TorrentTrackerData::Process()
        {
            std::lock_guard lock(mutex_);

            auto now = std::chrono::steady_clock::now();

            if (request_.has_value())
            {
                if (!request_->valid())
                {
                    if (now > timeout_)
                    {
                        *cancel_flag_ = true;
                        LOG("Manager", "Timed out waiting for tracker %s", tracker_list_.GetCurrentTracker().c_str());
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
            else if (tracker_interval_seconds_ != 0 && last_tracker_contact_ + std::chrono::seconds(tracker_interval_seconds_) < now)
                RequestTrackerUpdate();
        }

        void Manager::TorrentTrackerData::Cancel()
        {
            std::lock_guard lock(mutex_);

            *cancel_flag_ = true;
        }

        void Manager::TorrentTrackerData::SelectNextTracker()
        {
            std::lock_guard lock(mutex_);

            tracker_interval_seconds_ = 0;
            last_tracker_contact_ = {};
            tracker_list_.SelectNextTracker();
        }

        void Manager::TorrentTrackerData::RequestNewPeers(unsigned desired)
        {
            std::lock_guard lock(mutex_);

            request_parameters_->numwant = desired;

            if (!request_ && (tracker_interval_seconds_ == 0 || last_tracker_contact_ + std::chrono::seconds(tracker_interval_seconds_) < std::chrono::steady_clock::now()))
                RequestTrackerUpdate();
        }

        void Manager::TorrentTrackerData::RequestTrackerUpdate()
        {
            std::lock_guard lock(mutex_);

            while (!request_ && !tracker_list_.GetCurrentTracker().empty())
            {
                try
                {
                    const auto current_tracker = tracker_list_.GetCurrentTracker();
                    LOG("Manager", "Requesting tracker update from %s", current_tracker.c_str());
                    request_ = Announce(current_tracker, request_parameters_, cancel_flag_);
                    timeout_ = std::chrono::steady_clock::now() + std::chrono::seconds(60);
                }
                catch (UnsupportedProtocolException &e)
                {
                    LOG("Manager", "Unsupported protocol: %s", e.what());
                    tracker_list_.RemoveCurrentTracker();
                    SelectNextTracker();
                }
            }
        }

        bool Manager::TorrentTrackerData::HandleTrackerResult(const std::optional<AnnounceResponse> &result)
        {
            std::lock_guard lock(mutex_);

            if (!result)
            {
                LOG("Manager", "Request failed");
                return false;
            }

            if (result->failure_reason)
            {
                LOG("Manager", "Request failed: %s", result->failure_reason->c_str());
                return false;
            }

            LOG("Manager", "Request succeeded");
            LOG("Manager", "Peers:");
            for (const auto &peer : result->peers)
            {
                (void)peer;
                LOG("Manager", "  %s:%d", peer.ip.c_str(), peer.port);
            }
            callback(result->peers);

            tracker_interval_seconds_ = result->min_interval ? result->min_interval.value() : result->interval;

            if (result->tracker_id.has_value())
            {
                // TODO
            }

            if (last_tracker_contact_ != std::chrono::steady_clock::time_point{})
                request_parameters_->event = AnnounceParameters::Event::None;

            last_tracker_contact_ = std::chrono::steady_clock::now();
            return true;
        }

        void Manager::RegisterTorrent(const Torrent &torrent, std::function<void(const std::vector<PeerInfo> &)> callback)
        {
            std::lock_guard lock(mutex_);

            const auto it = std::find_if(torrents_.begin(), torrents_.end(), [&torrent](const std::unique_ptr<TorrentTrackerData> &data)
                                         { return data->torrent == &torrent; });

            if (it != torrents_.end())
            {
                (*it)->callback = std::move(callback);
                return;
            }

            torrents_.push_back(std::make_unique<TorrentTrackerData>(torrent, callback));
        }

        void Manager::UnregisterTorrent(const Torrent &torrent)
        {
            std::lock_guard lock(mutex_);

            const auto it = std::find_if(torrents_.begin(), torrents_.end(), [&torrent](const std::unique_ptr<TorrentTrackerData> &data)
                                         { return data->torrent == &torrent; });

            if (it != torrents_.end())
                torrents_.erase(it);
        }

        void Manager::RequestPeers(const Torrent &torrent, unsigned desired)
        {
            std::lock_guard lock(mutex_);

            const auto it = std::find_if(torrents_.begin(), torrents_.end(), [&torrent](const std::unique_ptr<TorrentTrackerData> &data)
                                         { return data->torrent == &torrent; });

            if (it != torrents_.end())
                (*it)->RequestNewPeers(desired);
        }
    }
}