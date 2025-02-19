#include "piece_manager.hpp"

#include <algorithm>
#include <cassert>
#include <random>

#include <tortoise/exceptions.hpp>

#include "../util/util.hpp"

namespace
{
	std::random_device rd;
	std::mt19937 g(rd());
}

namespace tortoise
{
	PieceManager::PieceManager(std::shared_ptr<const Metainfo> metainfo) : metainfo_(std::move(metainfo)), bitfield_(metainfo_->pieces.size())
	{
		std::size_t total_length = 0;
		for (const auto& file : metainfo_->files)
			total_length += file.length;

		pieces_.reserve(metainfo_->pieces.size());

		// Every piece is of equal length except possibly the last
		for (std::size_t i = 0; i < metainfo_->pieces.size() - 1; ++i)
			pieces_.emplace_back(static_cast<std::uint32_t>(i), metainfo_->piece_length);

		pieces_.emplace_back(static_cast<std::uint32_t>(metainfo_->pieces.size() - 1), static_cast<std::uint32_t>(total_length % metainfo_->piece_length));

		for (std::size_t i = 0; i < pieces_.size(); ++i)
		{
			auto& piece = pieces_.at(i);
			auto piece_blocks = piece.length / PieceManager::BLOCK_SIZE;
			const auto last_block_size = piece.length % PieceManager::BLOCK_SIZE;
			if (last_block_size > 0)
				++piece_blocks;

			piece.blocks.resize(piece_blocks);
			for (std::size_t j = 0; j < piece_blocks - 1; ++j)
				piece.blocks.at(j) = BlockData{ {static_cast<std::uint32_t>(i), static_cast<std::uint32_t>(j * PieceManager::BLOCK_SIZE), PieceManager::BLOCK_SIZE}, {} };
			piece.blocks.back() = BlockData{ {static_cast<std::uint32_t>(i), static_cast<std::uint32_t>((piece_blocks - 1) * PieceManager::BLOCK_SIZE), last_block_size ? last_block_size : PieceManager::BLOCK_SIZE}, {} };
		}
	}

	PieceManager::~PieceManager() = default;

	bool PieceManager::HaveInterestingPiece(Peer* peer)
	{
		(void)peer;
		return true; // TODO
	}

	const Bitfield& PieceManager::GetBitfield() const
	{
		return bitfield_;
	}

	void PieceManager::RegisterPeer(Peer* peer)
	{
		const auto [it, inserted] = peers_.insert({ peer, PeerData{Bitfield(pieces_.size()), {}} });
		(void)it;
		(void)inserted;
		assert(inserted); // We should not have the peer already
	}

	void PieceManager::UnregisterPeer(Peer* peer)
	{
		// TODO cancel all requests
		peers_.erase(peer);
	}

	void PieceManager::SetPeerBitfield(Peer* peer, Bitfield bitfield)
	{
		auto it = peers_.find(peer);
		if (it == peers_.end())
			return;

		it->second.bitfield = std::move(bitfield);

		for (const auto& piece : pieces_)
		{
			if (it->second.bitfield.HasPiece(piece.index))
				it->second.piece_queue.push_back(piece.index);
		}

		std::shuffle(it->second.piece_queue.begin(), it->second.piece_queue.end(), g);
	}

	void PieceManager::SetPeerHave(Peer* peer, std::uint32_t piece)
	{
		auto it = peers_.find(peer);
		if (it == peers_.end())
			return;

		it->second.bitfield.SetHavePiece(piece);

		auto& queue = it->second.piece_queue;
		if (queue.empty())
		{
			queue.push_back(piece);
			return;
		}
		const auto random_index = std::uniform_int_distribution<std::size_t>(0, queue.size() - 1)(g);
		auto& random_piece = queue.at(random_index);
		queue.push_back(random_piece);
		random_piece = piece;
	}

	std::vector<Block> PieceManager::GetRequests(Peer* peer)
	{
		const auto it = peers_.find(peer);
		if (it == peers_.end())
			throw InvalidArgumentException("Peer not registered");

		// TODO This should be improved (rarest first, endgame mode, ...).
		// The only policy we try to follow for now is to request blocks from the same piece such that we complete the piece as fast as possible.
		std::vector<Block> blocks;
		auto& queue = it->second.piece_queue;
		std::size_t requested_piece_counter = 0;
		while (blocks.empty() && !queue.empty() && requested_piece_counter < queue.size())
		{
			const auto piece_index = queue.back();
			auto& piece = pieces_.at(piece_index);
			if (piece.done)
			{
				queue.pop_back();
				continue;
			}

			if (piece.requested)
			{
				const auto first = queue.at(requested_piece_counter);
				queue.at(requested_piece_counter++) = piece_index;
				queue.back() = first;
				continue;
			}

			for (const auto& block : piece.blocks)
				blocks.push_back(block.block);

			piece.requested = true;
		}

		return blocks;
	}

	void PieceManager::ReceiveBlock(Peer* peer, std::uint32_t piece_index, std::uint32_t offset, ByteVector block)
	{
		(void)peer;
		(void)piece_index;
		(void)offset;
		(void)block;
	}

} // namespace tortoise