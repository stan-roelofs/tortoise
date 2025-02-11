#ifndef TORTOISE_PEER_HPP
#define TORTOISE_PEER_HPP

#include <tortoise/peer_info.hpp>

#include "peer_connection.hpp"

namespace tortoise
{
	class Peer
	{
	public:
		Peer(const PeerInfo& peer_info, std::shared_ptr<const Metainfo> metainfo, PeerId peer_id);
		~Peer();

		const PeerInfo& GetPeerInfo() const;
		PeerStatus GetStatus() const;
		PeerStatus Process();

	private:
		void OnMessageChoke();
		void OnMessageUnchoke();
		void OnMessageInterested();
		void OnMessageNotInterested();
		void OnMessageHave(std::uint32_t piece_index);
		void OnMessageBitfield(const ByteVector& bitfield);
		void OnMessageRequest(std::uint32_t index, std::uint32_t begin, std::uint32_t length);
		void OnMessagePiece(std::uint32_t index, std::uint32_t begin, const ByteVector& piece);
		void OnMessageCancel(std::uint32_t index, std::uint32_t begin, std::uint32_t length);
		void OnMessagePort(std::uint16_t port);

		PeerConnection connection_;
		PeerConnection::Status connection_status_;
		PeerStatus status_;

		bool am_choking_;      // This client is choking the peer
		bool am_interested_;   // This client is interested in the peer
		bool peer_choking_;    // Peer is choking this client
		bool peer_interested_; // Peer is interested in this client
	};
}

#endif