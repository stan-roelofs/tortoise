#ifndef TORTOISE_TORRENT_IMPL_HPP
#define TORTOISE_TORRENT_IMPL_HPP

#include <atomic>
#include <chrono>
#include <list>
#include <map>
#include <thread>

#include <tortoise/event.hpp>
#include <tortoise/peer_info.hpp>
#include <tortoise/torrent.hpp>

#include "../event_queue.hpp"
#include "peer.hpp"
#include "peer_connection.hpp"
#include "piece_manager.hpp"
#include "piece_writer.hpp"

namespace tortoise
{
	class Torrent : public std::enable_shared_from_this<Torrent>, public PieceManager::Listener
	{
	public:
		class PeerInfoProvider
		{
		public:
			virtual ~PeerInfoProvider() = default;
			virtual void RegisterTorrent(const Torrent& torrent, std::function<void(const std::vector<PeerInfo>&)> callback) = 0;
			virtual void UnregisterTorrent(const Torrent& torrent) = 0;
			virtual void RequestPeers(const Torrent& torrent, unsigned desired) = 0;
		};

		Torrent(const TorrentParameters& params, PeerInfoProvider& peer_info_provider, EventQueue& event_queue);
		~Torrent();

		bool RequestStart();
		void RequestStop();

		const Metainfo& GetMetainfo() const;
		PeerId GetPeerId() const;

		Statistics GetStatistics() const;

	private:
		void OnNewPeers(const std::vector<PeerInfo>& new_peers);
		void OnPieceDownloaded(std::uint32_t piece_index, std::shared_ptr<const ByteVector> data) override;
		void OnTorrentDownloaded();
		void OnWriteError();

		void RequestPeers();
		void UpdateStatistics();

		void Run(std::stop_token token);
		void ProcessPeers();

		const std::shared_ptr<const Metainfo> metainfo_;
		const PeerId peer_id_;

		EventQueue& event_queue_;
		PieceWriter piece_writer_;
		PieceManager piece_manager_;

		mutable std::recursive_mutex peer_mutex_;
		PeerInfoProvider& peer_info_provider_;
		std::chrono::steady_clock::time_point last_peer_request_;
		std::list<PeerInfo> potential_peers_;
		std::list<std::pair<std::unique_ptr<Peer>, PeerStatus>> peers_;

		mutable std::recursive_mutex data_mutex_;
		Statistics statistics_;
		bool started_;
		std::jthread thread_;
	};
}

#endif