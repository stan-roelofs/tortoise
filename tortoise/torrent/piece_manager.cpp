#include "piece_manager.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <random>

#include <tortoise/exception.hpp>

#include "../util/hash.hpp"
#include "../util/log.hpp"
#include "../util/util.hpp"

namespace
{
	std::random_device rd;
	std::mt19937 g(rd());
	const std::string log_tag = "PieceManager";
}

namespace tortoise
{
	PieceManager::PieceManager(std::shared_ptr<const Metainfo> metainfo)
		: metainfo_(std::move(metainfo)),
		  pieces_done_(0),
		  pieces_requested_(0),
		  endgame_mode_(false),
		  next_handle_(1),
		  bitfield_(metainfo_->pieces.size())
	{
		std::size_t total_length = 0;
		for (const auto &file : metainfo_->files)
			total_length += file.length;

		pieces_.reserve(metainfo_->pieces.size());

		// Every piece is of equal length except possibly the last
		for (std::size_t i = 0; i < metainfo_->pieces.size() - 1; ++i)
			pieces_.emplace_back(static_cast<std::uint32_t>(i), metainfo_->piece_length);

		pieces_.emplace_back(static_cast<std::uint32_t>(metainfo_->pieces.size() - 1), static_cast<std::uint32_t>(total_length % metainfo_->piece_length));

		for (std::size_t i = 0; i < pieces_.size(); ++i)
		{
			auto &piece = pieces_.at(i);
			auto piece_blocks = piece.length / PieceManager::BLOCK_SIZE;
			const auto last_block_size = piece.length % PieceManager::BLOCK_SIZE;
			if (last_block_size > 0)
				++piece_blocks;

			piece.blocks.resize(piece_blocks);
			for (std::size_t j = 0; j < piece_blocks - 1; ++j)
				piece.blocks.at(j) = BlockData{{static_cast<std::uint32_t>(i), static_cast<std::uint32_t>(j * PieceManager::BLOCK_SIZE), PieceManager::BLOCK_SIZE}, false, {}};
			piece.blocks.back() = BlockData{{static_cast<std::uint32_t>(i), static_cast<std::uint32_t>((piece_blocks - 1) * PieceManager::BLOCK_SIZE), last_block_size ? last_block_size : PieceManager::BLOCK_SIZE}, false, {}};
		}
	}

	PieceManager::~PieceManager() = default;

	bool PieceManager::HaveInterestingPiece(Handle peer)
	{
		(void)peer;
		return true; // TODO
	}

	const Bitfield &PieceManager::GetBitfield() const
	{
		return bitfield_;
	}

	PieceManager::Handle PieceManager::RegisterPeer()
	{
		Handle peer;
		if (!unused_handles_.empty())
		{
			peer = unused_handles_.front();
			unused_handles_.pop();
		}
		else
			peer = next_handle_++;

		const auto [it, inserted] = peers_.insert({peer, PeerData{Bitfield(pieces_.size()), {}}});
		(void)inserted;
		(void)it;
		assert(inserted); // We should not have the peer already
		return peer;
	}

	void PieceManager::UnregisterPeer(Handle peer)
	{
		// TODO cancel all requests
		peers_.erase(peer);
		unused_handles_.push(peer);
	}

	void PieceManager::SetPeerHave(Handle peer, std::uint32_t piece)
	{
		auto it = peers_.find(peer);
		if (it == peers_.end())
			return;

		it->second.bitfield.SetHavePiece(piece);

		auto &queue = it->second.piece_queue;
		if (queue.empty())
		{
			queue.push_back(piece);
			return;
		}
		const auto random_index = std::uniform_int_distribution<std::size_t>(0, queue.size() - 1)(g);
		auto &random_piece = queue.at(random_index);
		queue.push_back(random_piece);
		random_piece = piece;
	}

	void PieceManager::SetPeerBitfield(Handle peer, Bitfield bitfield)
	{
		auto it = peers_.find(peer);
		if (it == peers_.end())
			return;
		it->second.bitfield = std::move(bitfield);

		for (const auto &piece : pieces_)
		{
			if (it->second.bitfield.HasPiece(piece.index))
				it->second.piece_queue.push_back(piece.index);
		}

		std::shuffle(it->second.piece_queue.begin(), it->second.piece_queue.end(), g);
	}

	std::vector<Block> PieceManager::GetRequests(Handle peer)
	{
		const auto it = peers_.find(peer);
		if (it == peers_.end())
			throw InvalidArgumentException("Peer not registered");

		// TODO This should be improved (rarest first, endgame mode, ...).
		// The only policy we try to follow for now is to request blocks from the same piece such that we complete the piece as fast as possible.
		std::vector<Block> blocks;
		auto &queue = it->second.piece_queue;
		std::size_t requested_piece_counter = 0;
		while (blocks.empty() && !queue.empty() && requested_piece_counter < queue.size())
		{
			const auto piece_index = queue.back();
			auto &piece = pieces_.at(piece_index);
			if (piece.Finished())
			{
				queue.pop_back();
				continue;
			}

			if (piece.requested && !endgame_mode_)
			{
				const auto first = queue.at(requested_piece_counter);
				queue.at(requested_piece_counter++) = piece_index;
				queue.back() = first;
				continue;
			}

			queue.pop_back();

			for (const auto &block : piece.blocks)
			{
				if (!block.done)
					blocks.push_back(block.block);
			}

			if (!piece.requested)
			{
				++pieces_requested_;
				if (!endgame_mode_ && pieces_requested_ == metainfo_->pieces.size())
				{
					endgame_mode_ = true;
					LOG_INFO(log_tag, "Endgame mode enabled");
				}
			}
			piece.requested = true;
		}

		return blocks;
	}

	void PieceManager::ReceiveBlock(Handle, std::uint32_t piece_index, std::uint32_t offset, ByteVector block)
	{
		auto &piece = pieces_.at(piece_index);
		if (!piece.SetBlockData(offset, block))
		{
			LOG_WARN(log_tag, std::format("Already have block {} in piece {}", offset, piece_index));
			return;
		}

		if (!piece.Finished())
			return;

		const auto piece_data = piece.ReleaseBlockData();
		const auto hash = hash::CreateSHA1(piece_data->data(), piece_data->size());
		if (memcmp(hash.data(), metainfo_->pieces.at(piece_index).data(), hash.size() != 0))
		{
			LOG_WARN(log_tag, std::format("Hash mismatch in piece {}", piece_index));
			piece.Reset();
			return;
		}

		++pieces_done_;
		for (const auto &listener : listeners_)
			listener->OnPieceDownloaded(piece_index, piece_data);
	}

	void PieceManager::AddListener(Listener *listener)
	{
		listeners_.insert(listener);
	}

	void PieceManager::RemoveListener(Listener *listener)
	{
		listeners_.erase(listener);
	}

} // namespace tortoise