#ifndef TORTOISE_TRACKER_MANAGER_HPP
#define TORTOISE_TRACKER_MANAGER_HPP

#include <chrono>
#include <functional>
#include <map>
#include <list>
#include <thread>

#include <tortoise/sha1_hash.hpp>

#include "tracker_announce.hpp"
#include "tracker_connection.hpp"
#include "tracker_list.hpp"

#include "torrent_impl.hpp"

namespace tortoise
{
    //! \brief Manages the tracker requests and responses. Implements the multi-tracker logic as described in BEP 12.
    class TrackerManager : public Torrent::PeerInfoProvider
    {
    public:
        TrackerManager();
        ~TrackerManager();

        // Inherited via PeerInfoProvider
        virtual void RequestPeers(const Torrent &torrent, std::function<void(const PeerInfo &)> callback) override;
        virtual void CancelRequest(const Torrent &torrent) override;

    private:
        static void Run(TrackerManager &tracker_manager);

        class TorrentTrackerData
        {
        public:
            TorrentTrackerData(const Torrent &torrent, std::function<void(const PeerInfo &)> callback);
            ~TorrentTrackerData();

            void Process();
            void Cancel();

            const Torrent *torrent;

        private:
            void RequestTrackerUpdate();
            void SelectNextTracker();
            bool HandleTrackerResult(const std::optional<AnnounceResponse> &result);

            std::function<void(const PeerInfo &)> callback_;

            std::chrono::steady_clock::time_point last_tracker_contact_;
            std::chrono::steady_clock::time_point timeout_;
            std::uint64_t tracker_interval_seconds_;

            TrackerList tracker_list_;

            std::optional<std::future<std::optional<AnnounceResponse>>> request_;
            std::shared_ptr<std::atomic_bool> cancel_flag_;
        };

        std::list<std::unique_ptr<TorrentTrackerData>> torrents_;

        std::thread thread_;
        std::mutex mutex_;
        std::atomic_bool running_;
    };
}

#endif