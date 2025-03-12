#include "peer_connection.hpp"

#include <algorithm>
#include <cassert>
#include <random>
#include <vector>

#include <tortoise/exception.hpp>
#include <tortoise/metainfo.hpp>
#include <tortoise/peer_id.hpp>

#include "../util/log.hpp"

namespace
{
	// TODO make this configurable
	constexpr std::chrono::seconds keep_alive_timeout(120);
	constexpr std::chrono::seconds connect_timeout(30);
	constexpr std::chrono::seconds handshake_timeout = connect_timeout;

	constexpr std::chrono::seconds max_idle_time(keep_alive_timeout - std::chrono::seconds(5));

	constexpr static std::chrono::seconds speed_update_interval = std::chrono::seconds(2);

	const std::string_view log_tag = "PeerConnection";
}

namespace tortoise
{
	namespace protocol
	{
		enum class MessageID : std::uint8_t
		{
			Choke = 0,
			Unchoke = 1,
			Interested = 2,
			NotInterested = 3,
			Have = 4,
			Bitfield = 5,
			Request = 6,
			Piece = 7,
			Cancel = 8,
			Port = 9,
		};

		constexpr char protocol_string[] = "BitTorrent protocol";
		constexpr int protocol_string_size = sizeof(protocol_string) - 1;
		static_assert(protocol_string_size == 19, "Protocol string size is not 19 bytes.");
		constexpr int handshake_size = 49 + protocol_string_size;
		constexpr int length_prefix_size = 4; // The number of bytes used to prefix the length of a message
	}

	static std::uint32_t GetLengthPrefix(const ByteVector &buffer)
	{
		assert(buffer.size() >= protocol::length_prefix_size);
		if (buffer.size() < protocol::length_prefix_size)
			return 0;
		return network::NetworkToHost(util::Read<std::uint32_t>(buffer, 0));
	}

	static void WriteRequestMessage(ByteVector &buffer, protocol::MessageID type, std::uint32_t index, std::uint32_t begin, std::uint32_t length)
	{
		assert(type == protocol::MessageID::Request || type == protocol::MessageID::Cancel);
		const auto start_size = buffer.size();
		util::Write(buffer, network::HostToNetwork(static_cast<uint32_t>(13)));
		buffer.push_back(static_cast<std::uint8_t>(type));
		util::Write(buffer, network::HostToNetwork(index));
		util::Write(buffer, network::HostToNetwork(begin));
		util::Write(buffer, network::HostToNetwork(length));
		(void)start_size;
		assert(buffer.size() - start_size == 17);
	}

	static void WriteSimpleMessage(ByteVector &buffer, protocol::MessageID id)
	{
		assert(id == protocol::MessageID::Choke || id == protocol::MessageID::Unchoke || id == protocol::MessageID::Interested || id == protocol::MessageID::NotInterested);
		const auto start_size = buffer.size();
		util::Write(buffer, network::HostToNetwork(static_cast<std::uint32_t>(1)));
		buffer.push_back(static_cast<std::uint8_t>(id));
		assert(buffer.size() - start_size == 5);
		(void)start_size;
	}

	static void WriteInterestedMessage(ByteVector &buffer)
	{
		WriteSimpleMessage(buffer, protocol::MessageID::Interested);
	}

	static void WriteNotInterestedMessage(ByteVector &buffer)
	{
		WriteSimpleMessage(buffer, protocol::MessageID::NotInterested);
	}

	static void WriteKeepAliveMessage(ByteVector &buffer)
	{
		const auto start_size = buffer.size();
		util::Write(buffer, static_cast<std::uint32_t>(0));
		assert(buffer.size() - start_size == 4);
		(void)start_size;
	}

	static void WriteHaveMessage(ByteVector &buffer, std::uint32_t piece_index)
	{
		const auto start_size = buffer.size();
		util::Write(buffer, network::HostToNetwork(static_cast<std::uint32_t>(5)));
		buffer.push_back(static_cast<std::uint8_t>(protocol::MessageID::Have));
		util::Write(buffer, network::HostToNetwork(piece_index));
		assert(buffer.size() - start_size == 9);
		(void)start_size;
	}

	static void WriteBitfieldMessage(ByteVector &buffer, const ByteVector &bitfield)
	{
		const auto start_size = buffer.size();
		util::Write(buffer, network::HostToNetwork(static_cast<std::uint32_t>(1 + bitfield.size())));
		buffer.push_back(static_cast<std::uint8_t>(protocol::MessageID::Bitfield));
		buffer.insert(buffer.end(), bitfield.begin(), bitfield.end());
		assert(buffer.size() - start_size == 5 + bitfield.size());
		(void)start_size;
	}

	static void WriteHandshakeMessage(ByteVector &buffer, std::array<std::uint8_t, 20> info_hash, PeerId peer_id)
	{
		// handshake: <pstrlen><pstr><reserved><info_hash><peer_id>
		const auto start_size = buffer.size();
		(void)start_size;
		buffer.push_back(protocol::protocol_string_size);
		util::Write(buffer, protocol::protocol_string, protocol::protocol_string_size);
		constexpr std::uint8_t reserved[8] = {0};
		util::Write(buffer, reserved, 8);
		util::Write(buffer, info_hash.data(), info_hash.size());
		const auto peer = peer_id.Get();
		assert(peer.size() == 20);
		util::Write(buffer, peer.data(), peer.size());
		assert(buffer.size() - start_size == protocol::handshake_size);
	}

	static bool ParseHandshake(ByteVector &buffer, std::array<std::uint8_t, 20> &info_hash, PeerId &peer_id)
	{
		if (buffer.size() < protocol::handshake_size)
			return false;

		if (buffer[0] != protocol::protocol_string_size)
		{
			LOG_ERROR(log_tag, std::format("Invalid protocol string size: {}", buffer[0]));
			return false;
		}

		if (memcmp(&buffer[1], protocol::protocol_string, protocol::protocol_string_size) != 0)
		{
			LOG_ERROR(log_tag, "Invalid protocol string");
			return false;
		}

		std::copy(&buffer[28], &buffer[28] + 20, info_hash.begin());
		std::string peer_id_str(&buffer[48], &buffer[48] + 20);
		peer_id = PeerId::FromString(peer_id_str);
		return true;
	}

	// TODO: while handshaking make sure we don't connect to ourselves!

	PeerConnection::PeerConnection(PeerInfo peer_info, std::shared_ptr<const Metainfo> metainfo, PeerId peer_id, MessageCallbacks callbacks) : message_callbacks_(std::move(callbacks)),
																																			   peer_info_(peer_info),
																																			   metainfo_(std::move(metainfo)),
																																			   own_peer_id_(std::move(peer_id)),
																																			   can_receive_bitfield_(true),
																																			   status_(Status::Connecting),
																																			   socket_(network::TransportProtocol::TCP)
	{
		if (!metainfo_)
			throw InvalidArgumentException("Metainfo is null.");

		if (!message_callbacks_.bitfield || !message_callbacks_.cancel || !message_callbacks_.choke || !message_callbacks_.interested ||
			!message_callbacks_.not_interested || !message_callbacks_.piece || !message_callbacks_.port || !message_callbacks_.request ||
			!message_callbacks_.unchoke || !message_callbacks_.have)
			throw InvalidArgumentException("Missing message callbacks.");

		LOG_INFO(log_tag, std::format("Connecting to peer {}", peer_info_.ToString()));
		if (!socket_.Connect(peer_info_.ip, std::to_string(peer_info_.port)))
		{
			LOG_ERROR(log_tag, std::format("Error connecting to peer {}", peer_info_.ToString()));
			status_ = Status::Finished;
			return;
		}

		SetTimeout(connect_timeout);
	}

	PeerConnection::~PeerConnection() = default;

	std::uint64_t PeerConnection::GetDownloadSpeed() const { return speed_tracker_.download_speed_; }
	std::uint64_t PeerConnection::GetUploadSpeed() const { return speed_tracker_.upload_speed_; }

	PeerConnection::Status PeerConnection::Process()
	{
		if (status_ == Status::Finished)
			return status_; // We are done

		if (!CheckConnectionAlive())
			return status_;

		if (status_ == Status::Connecting && !Connect())
			return status_; // We are still connecting

		if (!Send())
		{
			Error(std::format("Error while sending data to peer {}", peer_info_.ToString()));
			return status_;
		}
		if (!Receive())
		{
			Error(std::format("Error while receiving data from peer {}", peer_info_.ToString()));
			return status_;
		}

		HandleMessages();
		UpdateSpeeds();
		return status_;
	}

	void PeerConnection::Disconnect()
	{
		LOG_INFO(log_tag, std::format("Disconnecting from peer {}", peer_info_.ToString()));
		status_ = Status::Finished;
	}

	void PeerConnection::SendRequest(std::uint32_t index, std::uint32_t begin, std::uint32_t length)
	{
		assert(length > 0);
		LOG_INFO(log_tag, std::format("Requesting block (index: {}, begin: {}, length: {}) from peer {}", index, begin, length, peer_info_.ToString()));
		WriteRequestMessage(send_buffer_, protocol::MessageID::Request, index, begin, length);
	}

	void PeerConnection::SendCancel(std::uint32_t index, std::uint32_t begin, std::uint32_t length)
	{
		assert(length > 0);
		LOG_INFO(log_tag, std::format("Cancelling block (index: {}, begin: {}, length: {}) from peer {}", index, begin, length, peer_info_.ToString()));
		WriteRequestMessage(send_buffer_, protocol::MessageID::Cancel, index, begin, length);
	}

	void PeerConnection::SendInterested()
	{
		LOG_INFO(log_tag, std::format("Sending interested to peer {}", peer_info_.ToString()));
		WriteInterestedMessage(send_buffer_);
	}

	void PeerConnection::SendNotInterested()
	{
		LOG_INFO(log_tag, std::format("Sending not interested to peer {}", peer_info_.ToString()));
		WriteNotInterestedMessage(send_buffer_);
	}

	void PeerConnection::SendHave(std::uint32_t piece_index)
	{
		LOG_INFO(log_tag, std::format("Sending have piece {} to peer {}", piece_index, peer_info_.ToString()));
		WriteHaveMessage(send_buffer_, piece_index);
	}

	void PeerConnection::SendBitfield(const ByteVector &bitfield)
	{
		LOG_INFO(log_tag, std::format("Sending bitfield to peer {}", peer_info_.ToString()));
		WriteBitfieldMessage(send_buffer_, bitfield);
	}

	PeerConnection::Status PeerConnection::GetStatus() const
	{
		return status_;
	}

	// Message handlers
	void PeerConnection::HandleMessages()
	{
		if (status_ == Status::Handshaking)
		{
			if (receive_buffer_.size() < protocol::handshake_size)
				return; // Not enough data, wait...

			std::array<std::uint8_t, 20> info_hash;
			PeerId peer_id;
			if (!ParseHandshake(receive_buffer_, info_hash, peer_id))
			{
				Error(std::format("Invalid handshake from peer {}", peer_info_.ToString()));
				return;
			}
			if (info_hash != metainfo_->info_hash)
			{
				Error(std::format("Invalid info hash from peer {}", peer_info_.ToString()));
				return;
			}
			if (peer_info_.peer_id && peer_id != *peer_info_.peer_id)
			{
				Error(std::format("Received invalid peer id from peer {}", peer_info_.ToString()));
				return;
			}
			LOG_INFO(log_tag, std::format("Successfully handshaked with peer {}", peer_info_.ToString()));

			ShiftBuffer(protocol::handshake_size);
			status_ = Status::Connected;
		}

		if (status_ != Status::Connected)
			return;

		// Handle messages as long as there is a complete message in the buffer
		while ((receive_buffer_.size() >= protocol::length_prefix_size) && ((receive_buffer_.size() - protocol::length_prefix_size) >= GetLengthPrefix(receive_buffer_)))
		{
			const auto length = GetLengthPrefix(receive_buffer_);
			if (length > 0)
			{
				switch (static_cast<protocol::MessageID>(receive_buffer_[protocol::length_prefix_size]))
				{
				case protocol::MessageID::Choke:
					message_callbacks_.choke();
					break;
				case protocol::MessageID::Unchoke:
					message_callbacks_.unchoke();
					break;
				case protocol::MessageID::Interested:
					message_callbacks_.interested();
					break;
				case protocol::MessageID::NotInterested:
					message_callbacks_.not_interested();
					break;
				case protocol::MessageID::Have:
					message_callbacks_.have(network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 1)));
					break;
				case protocol::MessageID::Bitfield:
				{
					if (!can_receive_bitfield_)
						break;

					auto bitfield = ByteVector(receive_buffer_.begin() + protocol::length_prefix_size + 1 /* skip message id */,
											   receive_buffer_.begin() + protocol::length_prefix_size + length);
					const std::size_t expected_bitfield_size = (metainfo_->pieces.size() + 7) / 8;
					if (bitfield.size() != expected_bitfield_size)
					{
						Error(std::format("Peer {} sent invalid bitfield", peer_info_.ToString()));
						return;
					}
					message_callbacks_.bitfield(std::move(bitfield));
					break;
				}
				case protocol::MessageID::Request:
				{
					const auto piece_index = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 1));
					const auto begin = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 5));
					const auto requested_length = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 9));
					message_callbacks_.request(piece_index, begin, requested_length);
					break;
				}
				case protocol::MessageID::Piece:
				{
					speed_tracker_.bytes_received_ += length;
					const auto piece_index = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 1));
					const auto begin = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 5));
					auto block = ByteVector(receive_buffer_.begin() + protocol::length_prefix_size + 9 /* skip message id, piece index and begin */,
											receive_buffer_.begin() + protocol::length_prefix_size + length);
					message_callbacks_.piece(piece_index, begin, std::move(block));
					break;
				}
				case protocol::MessageID::Cancel:
				{
					const auto piece_index = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 1));
					const auto begin = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 5));
					const auto requested_length = network::HostToNetwork(util::Read<std::uint32_t>(receive_buffer_, protocol::length_prefix_size + 9));
					message_callbacks_.cancel(piece_index, begin, requested_length);
					break;
				}
				case protocol::MessageID::Port:
				{
					const auto port = network::HostToNetwork(util::Read<std::uint16_t>(receive_buffer_, protocol::length_prefix_size + 1));
					message_callbacks_.port(port);
					break;
				}
				default:
					Error(std::format("Received invalid message from peer {}", peer_info_.ToString()));
					return;
				}
			}

			can_receive_bitfield_ = false; // We have received a message, so we can't receive a bitfield anymore
			ShiftBuffer(protocol::length_prefix_size + length);
		}
	}

	void PeerConnection::UpdateSpeeds()
	{
		const auto now = std::chrono::steady_clock::now();
		const auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - speed_tracker_.last_update_time_);
		if (duration < speed_update_interval)
			return;

		speed_tracker_.download_speed_ = speed_tracker_.bytes_received_ / duration.count();
		speed_tracker_.upload_speed_ = speed_tracker_.bytes_sent_ / duration.count();
		speed_tracker_.bytes_received_ = 0;
		speed_tracker_.bytes_sent_ = 0;

		speed_tracker_.last_update_time_ = now;
	}

	void PeerConnection::ShiftBuffer(std::size_t amount)
	{
		assert(amount <= receive_buffer_.size());
		if (amount > receive_buffer_.size())
			amount = receive_buffer_.size();
		receive_buffer_.erase(receive_buffer_.begin(), receive_buffer_.begin() + amount);
	}

	void PeerConnection::Error(const std::string &reason)
	{
		LOG_ERROR(log_tag, reason);
		status_ = Status::Finished;
	}

	bool PeerConnection::Connect()
	{
		assert(status_ == Status::Connecting);
		if (status_ != Status::Connecting)
			return false;

		switch (socket_.GetConnectionStatus())
		{
		case network::Socket::Status::Pending:
			return false;
		case network::Socket::Status::Connected:
		{
			SetTimeout(handshake_timeout);
			status_ = Status::Handshaking;
			WriteHandshakeMessage(send_buffer_, metainfo_->info_hash, own_peer_id_);
			LOG_INFO(log_tag, std::format("Connected to peer {}", peer_info_.ToString()));
			return true;
		}
		case network::Socket::Status::Error:
		{
			Error(std::format("Error connecting to peer {}", peer_info_.ToString()));
			return false;
		}
		}

		return false; // Not reached
	}

	void PeerConnection::SetTimeout(std::chrono::seconds timeout)
	{
		timeout_ = std::chrono::steady_clock::now() + timeout;
	}

	bool PeerConnection::CheckConnectionAlive()
	{
		const auto current_time = std::chrono::steady_clock::now();
		if (status_ == Status::Connected && current_time - time_last_sent_ > max_idle_time)
		{
			WriteKeepAliveMessage(send_buffer_);
			time_last_sent_ = current_time;
		}

		if (std::chrono::steady_clock::now() <= timeout_)
			return true;

		LOG_INFO(log_tag, std::format("Peer {} timed out", peer_info_.ToString()));
		status_ = Status::Finished;
		return false;
	}

	bool PeerConnection::Send()
	{
		if (send_buffer_.empty())
			return true;

		int length = (int)std::min(send_buffer_.size(), (std::size_t)std::numeric_limits<int>::max());
		switch (socket_.Send(send_buffer_.data(), length))
		{
		case network::Socket::Result::Ok:
			LOG_INFO(log_tag, std::format("Sent {} bytes to peer {}", length, peer_info_.ToString()));
			time_last_sent_ = std::chrono::steady_clock::now();
			send_buffer_.erase(send_buffer_.begin(), send_buffer_.begin() + length);
			return true;
		case network::Socket::Result::WouldBlock:
			return true;
		case network::Socket::Result::Error:
			return false;
			break;
		}

		return true;
	}

	bool PeerConnection::Receive()
	{
		unsigned received_bytes = 0;
		network::Socket::Result result = network::Socket::Result::Ok;
		while (result == network::Socket::Result::Ok)
		{
			std::uint8_t chunk[1024];
			int length = sizeof(chunk);

			result = socket_.Receive(chunk, length);
			if (result == network::Socket::Result::Error || result == network::Socket::Result::WouldBlock || length == 0)
				break;

			received_bytes += length;
			receive_buffer_.insert(receive_buffer_.end(), chunk, chunk + length);
		}

		if (received_bytes > 0)
		{
			SetTimeout(keep_alive_timeout);
			LOG_INFO(log_tag, std::format("Received {} bytes from peer {}", received_bytes, peer_info_.ToString()));
		}

		return result != network::Socket::Result::Error;
	}
}