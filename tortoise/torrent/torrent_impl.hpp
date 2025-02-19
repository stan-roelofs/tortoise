#ifndef TORTOISE_TORRENT_IMPL_HPP
#define TORTOISE_TORRENT_IMPL_HPP

#include <tortoise/event.hpp>
#include <tortoise/peer_info.hpp>
#include <tortoise/torrent.hpp>

#include "../event_queue.hpp"
#include "peer.hpp"
#include "peer_connection.hpp"
#include "piece_manager.hpp"

#include <atomic>
#include <list>
#include <map>
#include <thread>

namespace tortoise
{
	class Torrent : public std::enable_shared_from_this<Torrent>
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

		void Start();
		void Stop();

		const Metainfo& GetMetainfo() const;
		PeerId GetPeerId() const;

	private:
		void OnNewPeers(const std::vector<PeerInfo>& new_peers);

		static void Run(Torrent& torrent);
		void ProcessPeers();

		std::atomic_bool running_;
		mutable std::recursive_mutex mutex_;
		std::thread thread_;

		bool requested_peers_;
		PeerInfoProvider& peer_info_provider_;
		EventQueue& event_queue_;
		std::shared_ptr<const Metainfo> metainfo_;
		const PeerId peer_id_;
		PieceManager piece_manager_;

		std::list<PeerInfo> potential_peers_;

		std::list<std::pair<std::unique_ptr<Peer>, PeerStatus>> peers_;
	};
}

#endif