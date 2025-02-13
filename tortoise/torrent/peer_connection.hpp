#ifndef TORTOISE_PEER_CONNECTION_HPP
#define TORTOISE_PEER_CONNECTION_HPP

#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <thread>

#include <tortoise/metainfo.hpp>
#include <tortoise/peer_info.hpp>

#include "piece_manager.hpp"

#include "../network/socket.hpp"
#include "../util/util.hpp"

namespace tortoise
{
	//! \brief Establishes a connection to a peer and handles further communication.
	class PeerConnection
	{
	public:
		enum class Status
		{
			Connecting,
			Connected,
			Handshaking,
			Finished
		};

		struct MessageCallbacks
		{
			std::function<void()> choke;
			std::function<void()> unchoke;
			std::function<void()> interested;
			std::function<void()> not_interested;
			std::function<void(std::uint32_t)> have;
			std::function<void(const ByteVector &)> bitfield;
			std::function<void(std::uint32_t, std::uint32_t, std::uint32_t)> request;
			std::function<void(std::uint32_t, std::uint32_t, const ByteVector &)> piece;
			std::function<void(std::uint32_t, std::uint32_t, std::uint32_t)> cancel;
			std::function<void(std::uint16_t)> port;
		};

		/*!
		 * \brief Creates a new peer connection.
		 * \param peer_info The peer info that is required to connect.
		 * \param info_hash The info hash of the torrent, used for the handshake.
		 * \param peer_id The peer id of this client, used for the handshake.
		 */
		PeerConnection(const PeerInfo &peer_info, const Metainfo &metainfo, PeerId peer_id, MessageCallbacks callbacks);
		~PeerConnection();

		Status GetStatus() const;
		const PeerInfo &GetPeerInfo() const;

		Status Process();

	private:
		void HandleMessages();
		void ShiftBuffer(std::size_t amount);
		void Error(const std::string &reason);
		bool Connect();
		void SetTimeout(std::chrono::seconds timeout);
		bool CheckConnectionAlive();
		bool Send();
		bool Receive();

		MessageCallbacks message_callbacks_;

		PeerInfo peer_info_;
		const Metainfo &metainfo_;
		PeerId own_peer_id_;

		bool can_receive_bitfield_;

		Status status_;
		std::chrono::steady_clock::time_point timeout_;
		std::chrono::steady_clock::time_point time_last_sent_;
		network::Socket socket_;
		ByteVector send_buffer_;
		ByteVector receive_buffer_;
	};

}

#endif