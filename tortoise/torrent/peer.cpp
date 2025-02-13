#include "peer.hpp"

#include "../util/log.hpp"

namespace
{
	const std::string_view log_tag = "Peer";

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
	Peer::Peer(const PeerInfo& peer_info, std::shared_ptr<const Metainfo> metainfo, PeerId peer_id) : connection_(peer_info, metainfo, peer_id,
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
		peer_choking_(false),
		peer_interested_(false)
	{
	}
	Peer::~Peer() = default;

	const PeerInfo& Peer::GetPeerInfo() const
	{
		return connection_.GetPeerInfo();
	}

	PeerStatus Peer::GetStatus() const
	{
		return status_;
	}

	PeerStatus Peer::Process()
	{
		const auto status = connection_.Process();
		if (connection_status_ != status)
			status_ = ConnectionStatusToPeerStatus(status);
		return status_;
	}

	void Peer::OnMessageChoke()
	{
		LOG_INFO(log_tag, std::format("Peer {} choked us", connection_.GetPeerInfo().ToString()));
		peer_choking_ = true;
	}

	void Peer::OnMessageUnchoke()
	{
		LOG_INFO(log_tag, std::format("Peer {} unchoked us", connection_.GetPeerInfo().ToString()));
		peer_choking_ = false;
	}

	void Peer::OnMessageInterested()
	{
		LOG_INFO(log_tag, std::format("Peer {} is interested", connection_.GetPeerInfo().ToString()));
		peer_interested_ = true;
	}

	void Peer::OnMessageNotInterested()
	{
		LOG_INFO(log_tag, std::format("Peer {} is not interested", connection_.GetPeerInfo().ToString()));
		peer_interested_ = false;
	}

	void Peer::OnMessageHave(std::uint32_t piece_index)
	{
		LOG_INFO(log_tag, std::format("Peer {} has piece {}", connection_.GetPeerInfo().ToString(), piece_index));
	}

	void Peer::OnMessageBitfield(const ByteVector& bitfield)
	{

		LOG_INFO(log_tag, std::format("Peer {} sent bitfield", connection_.GetPeerInfo().ToString()));

		for (std::uint32_t byte = 0; byte < bitfield.size(); ++byte)
		{
			for (std::uint32_t bit = 0; bit < 8; ++bit)
			{
				if (bitfield[byte] & (1 << (7 - bit)))
				{
					// const std::uint32_t piece_index = (byte * 8) + bit;
				}
			}
		}
	}

	void Peer::OnMessageRequest(std::uint32_t index, std::uint32_t begin, std::uint32_t length)
	{
		LOG_INFO(log_tag, std::format("Peer {} requested piece {}, begin {}, length {}", connection_.GetPeerInfo().ToString(), index, begin, length));
	}

	void Peer::OnMessagePiece(std::uint32_t index, std::uint32_t begin, const ByteVector& piece)
	{
		LOG_INFO(log_tag, std::format("Peer {} sent piece {}, begin {}, length {}", connection_.GetPeerInfo().ToString(), index, begin, piece.size()));
	}

	void Peer::OnMessageCancel(std::uint32_t index, std::uint32_t begin, std::uint32_t length)
	{
		LOG_INFO(log_tag, std::format("Peer {} cancelled request piece {}, begin {}, length {}", connection_.GetPeerInfo().ToString(), index, begin, length));
	}

	void Peer::OnMessagePort(std::uint16_t port)
	{
		LOG_INFO(log_tag, std::format("Peer {} sent port {}", connection_.GetPeerInfo().ToString(), port));
	}
}