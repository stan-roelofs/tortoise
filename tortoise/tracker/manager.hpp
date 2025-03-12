#ifndef TORTOISE_TRACKER_MANAGER_HPP
#define TORTOISE_TRACKER_MANAGER_HPP

#include <chrono>
#include <functional>
#include <map>
#include <list>
#include <thread>

#include "announce.hpp"
#include "connection.hpp"
#include "tiered_list.hpp"

#include "../torrent/torrent_impl.hpp"

namespace tortoise
{
	namespace tracker
	{
		//! \brief Manages the tracker requests and responses. Implements the multi-tracker logic as described in BEP 12.
		class Manager : public Torrent::PeerInfoProvider
		{
		public:
			Manager();
			~Manager();

			// Inherited via PeerInfoProvider
			void RegisterTorrent(const Torrent &torrent, std::function<void(const std::vector<PeerInfo> &)> callback) override;
			void UnregisterTorrent(const Torrent &torrent) override;
			void RequestPeers(const Torrent &torrent, unsigned desired) override;

		private:
			static void Run(Manager &tracker_manager);

			class TorrentTrackerData
			{
			public:
				TorrentTrackerData(const Torrent &torrent, std::function<void(const std::vector<PeerInfo> &)> callback);
				~TorrentTrackerData();

				void Process();
				void Cancel();
				void RequestNewPeers(unsigned desired);

				const Torrent *torrent;
				std::function<void(const std::vector<PeerInfo> &)> callback;

			private:
				void RequestTrackerUpdate();
				void SelectNextTracker();
				bool HandleTrackerResult(const std::optional<AnnounceResponse> &result);

				std::recursive_mutex mutex_;

				std::chrono::steady_clock::time_point last_tracker_contact_;
				std::chrono::steady_clock::time_point timeout_;
				std::uint64_t tracker_interval_seconds_;

				tracker::TieredList tracker_list_;

				std::shared_ptr<AnnounceParameters> request_parameters_;
				std::optional<std::future<std::optional<AnnounceResponse>>> request_;
				std::shared_ptr<std::atomic_bool> cancel_flag_;
			};

			std::list<std::unique_ptr<TorrentTrackerData>> torrents_;

			std::atomic_bool running_;
			std::thread thread_;
			std::recursive_mutex mutex_;
		};
	}
}

#endif