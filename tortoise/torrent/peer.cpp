#include "peer.hpp"

#include <cassert>

#include <tortoise/exceptions.hpp>

#include "../util/log.hpp"

namespace
{
	const std::string_view log_tag = "Peer";

	constexpr unsigned DESIRED_REQUESTS = 10; // TODO: determine whether this is a good value

	tortoise::PeerStatus ConnectionStatusToPeerStatus(tortoise::PeerConnection::Status status)
	{
		switch (status)
		{
		case tortoise::PeerConnection::Status::Connecting:
		case tortoise::PeerConnection::Status::Handshaking:
			return tortoise::PeerStatus::Connecting;
		case tortoise::PeerConnection::Status::Connected:
			return tortoise::PeerStatus::Connected;
		case tortoise::PeerConnection::Status::Finished:
			return tortoise::PeerStatus::Disconnected;
		}

		throw tortoise::Exception("Invalid status");
	}
}

namespace tortoise
{
	Peer::Peer(PeerInfo peer_info, std::shared_ptr<const Metainfo> metainfo, PeerId peer_id, PieceManager& piece_manager) :
		metainfo_(metainfo),
		peer_info_(peer_info),
		connection_(peer_info, metainfo, peer_id,
			PeerConnection::MessageCallbacks(
				std::bind(&Peer::OnMessageChoke, this),
				std::bind(&Peer::OnMessageUnchoke, this),
				std::bind(&Peer::OnMessageInterested, this),
				std::bind(&Peer::OnMessageNotInterested, this),
				std::bind(&Peer::OnMessageHave, this, std::placeholders::_1),
				std::bind(&Peer::OnMessageBitfield, this, std::placeholders::_1),
				std::bind(&Peer::OnMessageRequest, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
				std::bind(&Peer::OnMessagePiece, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
				std::bind(&Peer::OnMessageCancel, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
				std::bind(&Peer::OnMessagePort, this, std::placeholders::_1))),
		connection_status_(PeerConnection::Status::Connecting),
		status_(PeerStatus::Connecting),
		am_choking_(true),
		am_interested_(false),
		peer_choking_(true),
		peer_interested_(false),
		piece_manager_(piece_manager),
		handle_(PieceManager::INVALID_HANDLE)
	{
		if (!metainfo)
			throw InvalidArgumentException("Metainfo is null");

		handle_ = piece_manager_.RegisterPeer();
	}

	Peer::~Peer()
	{
		piece_manager_.UnregisterPeer(handle_);
	}

	const PeerInfo& Peer::GetPeerInfo() const
	{
		return peer_info_;
	}

	PeerStatus Peer::GetStatus() const
	{
		return status_;
	}

	PeerStatus Peer::Process()
	{
		const auto status = connection_.Process();
		if (connection_status_ != status)
		{
			if (status == PeerConnection::Status::Connected)
			{
				assert(connection_status_ == PeerConnection::Status::Connecting || connection_status_ == PeerConnection::Status::Handshaking);
				connection_.SendBitfield(piece_manager_.GetBitfield().AsBytes());
			}
			status_ = ConnectionStatusToPeerStatus(status);
			connection_status_ = status;
		}

		if (connection_status_ != tortoise::PeerConnection::Status::Connected)
			return status_;

		UpdateInterested();

		if (peer_choking_ || requested_blocks_.size() >= DESIRED_REQUESTS)
			return status_;

		MakeRequests();

		return status_;
	}

	void Peer::UpdateInterested()
	{
		bool interested = false;
		if (!requested_blocks_.empty() || !request_queue_.empty())
			interested = true;
		else
			interested = piece_manager_.HaveInterestingPiece(handle_);

		if (am_interested_ != interested)
		{
			am_interested_ = interested;
			if (am_interested_)
				connection_.SendInterested();
			else
				connection_.SendNotInterested();
		}
	}

	void Peer::MakeRequests()
	{
		assert(requested_blocks_.size() < DESIRED_REQUESTS);

		const auto requests_to_send = DESIRED_REQUESTS - requested_blocks_.size();
		while (request_queue_.size() < requests_to_send)
		{
			const auto blocks = piece_manager_.GetRequests(handle_);
			if (blocks.empty())
				break;
			for (const auto& block : blocks)
				request_queue_.push(block);
		}

		while (!request_queue_.empty() && requested_blocks_.size() < DESIRED_REQUESTS)
		{
			const auto block = request_queue_.front();
			request_queue_.pop();

			requested_blocks_.insert(block);
			connection_.SendRequest(block.piece_index, block.offset, block.length);
		}
	}

	void Peer::OnPieceDownloaded(std::uint32_t piece_index)
	{
		if (connection_status_ != PeerConnection::Status::Connected)
			return;
		connection_.SendHave(piece_index);
	}

	void Peer::OnMessageChoke()
	{
		LOG_INFO(log_tag, std::format("Peer {} choked us", peer_info_.ToString()));
		peer_choking_ = true;
	}

	void Peer::OnMessageUnchoke()
	{
		LOG_INFO(log_tag, std::format("Peer {} unchoked us", peer_info_.ToString()));
		peer_choking_ = false;
	}

	void Peer::OnMessageInterested()
	{
		LOG_INFO(log_tag, std::format("Peer {} is interested", peer_info_.ToString()));
		peer_interested_ = true;
	}

	void Peer::OnMessageNotInterested()
	{
		LOG_INFO(log_tag, std::format("Peer {} is not interested", peer_info_.ToString()));
		peer_interested_ = false;
	}

	void Peer::OnMessageHave(std::uint32_t piece_index)
	{
		LOG_INFO(log_tag, std::format("Peer {} has piece {}", peer_info_.ToString(), piece_index));

		piece_manager_.SetPeerHave(handle_, piece_index);
	}

	void Peer::OnMessageBitfield(ByteVector bitfield)
	{
		LOG_INFO(log_tag, std::format("Peer {} sent bitfield", peer_info_.ToString()));

		Bitfield peer_bitfield(metainfo_->pieces.size());
		peer_bitfield.FromBytes(std::move(bitfield));

		piece_manager_.SetPeerBitfield(handle_, std::move(peer_bitfield));
	}

	void Peer::OnMessageRequest(std::uint32_t index, std::uint32_t begin, std::uint32_t length)
	{
		LOG_INFO(log_tag, std::format("Peer {} requested block {}, begin {}, length {}", peer_info_.ToString(), index, begin, length));

		// TODO: only allow 2^14 or 2^15 block sizes, unless this is the last block in which case it can be smaller
	}

	void Peer::OnMessagePiece(std::uint32_t index, std::uint32_t begin, ByteVector piece)
	{
		const auto it = requested_blocks_.find(Block{ index, begin, static_cast<std::uint32_t>(piece.size()) });
		if (it == requested_blocks_.end() || it->length != static_cast<std::uint32_t>(piece.size()))
		{
			LOG_WARN(log_tag, std::format("Peer {} sent invalid block {}, begin {}, length {}", peer_info_.ToString(), index, begin, piece.size()));
			return;
		}

		LOG_INFO(log_tag, std::format("Peer {} sent block {}, begin {}, length {}", peer_info_.ToString(), index, begin, piece.size()));
		requested_blocks_.erase(Block{ index, begin, 0 });
		piece_manager_.ReceiveBlock(handle_, index, begin, std::move(piece));
	}

	void Peer::OnMessageCancel(std::uint32_t index, std::uint32_t begin, std::uint32_t length)
	{
		LOG_INFO(log_tag, std::format("Peer {} cancelled request {}, begin {}, length {}", peer_info_.ToString(), index, begin, length));
	}

	void Peer::OnMessagePort(std::uint16_t port)
	{
		LOG_INFO(log_tag, std::format("Peer {} sent port {}", peer_info_.ToString(), port));
	}
}