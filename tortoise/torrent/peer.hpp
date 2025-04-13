#ifndef TORTOISE_PEER_HPP
#define TORTOISE_PEER_HPP

#include <set>

#include <tortoise/peer_info.hpp>
#include <tortoise/torrent_parameters.hpp>

#include "bitfield.hpp"
#include "peer_connection.hpp"
#include "piece_manager.hpp"

namespace tortoise
{
	class Peer : public PieceManager::Listener
	{
	public:
		Peer(PeerInfo peer_info, std::shared_ptr<const TorrentParameters> torrent_parameters, std::shared_ptr<const Metainfo> metainfo, PeerId peer_id, PieceManager &piece_manager);
		~Peer();

		const PeerInfo &GetPeerInfo() const;
		PeerStatus GetStatus() const;
		PeerStatus Process();

		//! \returns the download speed in bytes per second
		std::uint64_t GetDownloadSpeed() const;
		//! \returns the upload speed in bytes per second
		std::uint64_t GetUploadSpeed() const;

	private:
		void OnMessageChoke();
		void OnMessageUnchoke();
		void OnMessageInterested();
		void OnMessageNotInterested();
		void OnMessageHave(std::uint32_t piece_index);
		void OnMessageBitfield(ByteVector bitfield);
		void OnMessageRequest(std::uint32_t index, std::uint32_t begin, std::uint32_t length);
		void OnMessagePiece(std::uint32_t index, std::uint32_t begin, ByteVector piece);
		void OnMessageCancel(std::uint32_t index, std::uint32_t begin, std::uint32_t length);
		void OnMessagePort(std::uint16_t port);

		void UpdateInterested();
		void MakeRequests();

		// Inherited via Listener
		void OnPieceDownloaded(std::uint32_t piece_index, std::shared_ptr<const ByteVector> data) override;

		std::shared_ptr<const TorrentParameters> torrent_parameters_;
		std::shared_ptr<const Metainfo> metainfo_;
		PeerInfo peer_info_;
		PeerConnection connection_;
		PeerConnection::Status connection_status_;
		PeerStatus status_;

		bool am_choking_;	   // This client is choking the peer
		bool am_interested_;   // This client is interested in the peer
		bool peer_choking_;	   // Peer is choking this client
		bool peer_interested_; // Peer is interested in this client

		PieceManager &piece_manager_;
		PieceManager::Handle handle_;
		std::queue<Block> request_queue_;
		std::set<Block> requested_blocks_;
	};
}

#endif