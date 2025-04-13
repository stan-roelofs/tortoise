#ifndef TORTOISE_PIECE_MANAGER_HPP
#define TORTOISE_PIECE_MANAGER_HPP

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <set>
#include <vector>

#include <tortoise/metainfo.hpp>

#include "bitfield.hpp"

namespace tortoise
{
	struct Block
	{
		std::uint32_t piece_index;
		std::uint32_t offset;
		std::uint32_t length;

		bool operator<(const Block& other) const
		{
			return piece_index < other.piece_index || (piece_index == other.piece_index && offset < other.offset);
		}
	};

	// struct Block
	// {
	// 	std::uint32_t piece_index;
	// 	std::uint32_t offset;
	// 	std::uint32_t length;

	// 	bool operator==(const Block &other) const
	// 	{
	// 		return piece_index == other.piece_index && offset == other.offset && length == other.length;
	// 	}

	// 	bool operator!=(const Block &other) const
	// 	{
	// 		return !(*this == other);
	// 	}

	// };

	class PieceManager
	{
	public:
		static constexpr std::uint32_t BLOCK_SIZE = 1 << 14; // 2^14 is used by near all clients and some even enforce this size.
		using Handle = std::uint32_t;
		static constexpr Handle INVALID_HANDLE = 0;

		class Listener
		{
		public:
			virtual void OnPieceDownloaded(std::uint32_t piece_index, std::shared_ptr<const ByteVector> data) = 0;
		};

		PieceManager(std::shared_ptr<const Metainfo> metainfo);
		~PieceManager();

		bool HaveInterestingPiece(Handle peer);

		const Bitfield& GetBitfield() const;

		Handle RegisterPeer();
		void UnregisterPeer(Handle peer);

		void SetPeerHave(Handle peer, std::uint32_t piece);
		void SetPeerBitfield(Handle peer, Bitfield bitfield);
		std::vector<Block> GetRequests(Handle peer);
		void ReceiveBlock(Handle peer, std::uint32_t piece_index, std::uint32_t offset, ByteVector block);

		void AddListener(Listener* listener);
		void RemoveListener(Listener* listener);

	private:
		struct BlockData
		{
			Block block;
			bool done;
			ByteVector data;
		};
		struct Piece
		{
			Piece(std::uint32_t index, std::uint32_t length) : index(index), length(length), blocks_done(false), requested(false)
			{
			}

			bool Finished() const { return blocks_done == blocks.size(); }
			bool SetBlockData(std::uint32_t offset, ByteVector data)
			{
				auto& block = blocks.at(offset / BLOCK_SIZE);
				if (block.done)
					return false;

				++blocks_done;
				block.data = std::move(data);
				block.done = true;
				return true;
			}

			void Reset()
			{
				blocks_done = 0;
				for (auto& block : blocks)
				{
					block.done = false;
					block.data.clear();
				}
				requested = false;
			}

			std::shared_ptr<ByteVector> ReleaseBlockData()
			{
				std::shared_ptr<ByteVector> data = std::make_shared<ByteVector>();
				data->reserve(length);
				for (auto& block : blocks)
				{
					std::copy(block.data.begin(), block.data.end(), std::back_inserter(*data));
					block = {};
				}
				return data;
			}

			std::uint32_t index;
			std::uint32_t length;
			std::vector<BlockData> blocks;
			std::size_t blocks_done;
			bool requested;
		};
		struct PeerData
		{
			Bitfield bitfield;
			std::vector<uint32_t> piece_queue;
		};

		const std::shared_ptr<const Metainfo> metainfo_;

		std::vector<Piece> pieces_;
		std::size_t pieces_done_;
		std::size_t pieces_requested_;
		bool endgame_mode_;
		std::map<Handle, PeerData> peers_;

		std::set<Listener*> listeners_;

		std::queue<Handle> unused_handles_;
		Handle next_handle_;

		Bitfield bitfield_;
	};
} // namespace tortoise

#endif