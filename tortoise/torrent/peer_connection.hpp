#ifndef TORTOISE_PEER_CONNECTION_HPP
#define TORTOISE_PEER_CONNECTION_HPP

#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <thread>

#include <tortoise/metainfo.hpp>
#include <tortoise/peer_info.hpp>
#include <tortoise/torrent_parameters.hpp>

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
			Handshaking,
			Connected,
			Finished
		};

		struct MessageCallbacks
		{
			std::function<void()> choke;
			std::function<void()> unchoke;
			std::function<void()> interested;
			std::function<void()> not_interested;
			std::function<void(std::uint32_t)> have;
			std::function<void(ByteVector)> bitfield;
			std::function<void(std::uint32_t, std::uint32_t, std::uint32_t)> request;
			std::function<void(std::uint32_t, std::uint32_t, ByteVector)> piece;
			std::function<void(std::uint32_t, std::uint32_t, std::uint32_t)> cancel;
			std::function<void(std::uint16_t)> port;
		};

		/*!
		 * \brief Creates a new peer connection.
		 * \param peer_info The peer info that is required to connect.
		 * \param torrent_parameters The parameters of the torrent.
		 * \param info_hash The info hash of the torrent, used for the handshake.
		 * \param peer_id The peer id of this client, used for the handshake.
		 */
		PeerConnection(PeerInfo peer_info, std::shared_ptr<const TorrentParameters> torrent_parameters, std::shared_ptr<const Metainfo> metainfo, PeerId peer_id, MessageCallbacks callbacks);
		~PeerConnection();

		Status GetStatus() const;
		//! \returns the download speed in bytes per second
		std::uint64_t GetDownloadSpeed() const;
		//! \returns the upload speed in bytes per second
		std::uint64_t GetUploadSpeed() const;

		Status Process();

		void Disconnect();

		void SendRequest(std::uint32_t index, std::uint32_t begin, std::uint32_t length);
		void SendInterested();
		void SendNotInterested();
		void SendHave(std::uint32_t piece_index);
		void SendCancel(std::uint32_t index, std::uint32_t begin, std::uint32_t length);
		void SendBitfield(const ByteVector &bitfield);

	private:
		void HandleMessages();
		void UpdateSpeeds();
		void ShiftBuffer(std::size_t amount);
		void Error(const std::string &reason);
		bool Connect();
		void SetTimeout(std::chrono::seconds timeout);
		bool CheckConnectionAlive();
		bool Send();
		bool Receive();

		MessageCallbacks message_callbacks_;

		std::shared_ptr<const TorrentParameters> torrent_parameters_;
		PeerInfo peer_info_;
		std::shared_ptr<const Metainfo> metainfo_;
		PeerId own_peer_id_;

		bool can_receive_bitfield_;

		Status status_;
		std::chrono::steady_clock::time_point timeout_;
		std::chrono::steady_clock::time_point time_last_sent_;
		network::Socket socket_;
		ByteVector send_buffer_;
		ByteVector receive_buffer_;

		struct
		{
			std::chrono::steady_clock::time_point last_update_time_ = std::chrono::steady_clock::now();
			std::uint64_t bytes_sent_ = 0;
			std::uint64_t bytes_received_ = 0;
			std::uint64_t upload_speed_ = 0;
			std::uint64_t download_speed_ = 0;
		} speed_tracker_;
	};

}

#endif