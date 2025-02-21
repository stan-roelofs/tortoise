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
			virtual void OnPieceDownloaded(std::uint32_t piece_index) = 0;
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
				auto& block = blocks.at(offset / BLOCK_SIZE).data;
				if (!block.empty())
					return false;

				++blocks_done;
				block = std::move(data);
				return true;
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

		std::vector<Piece> pieces_;
		std::map<Handle, PeerData> peers_;

		std::set<Listener*> listeners_;

		std::queue<Handle> unused_handles_;
		Handle next_handle_;

		std::shared_ptr<const Metainfo> metainfo_;
		Bitfield bitfield_;
	};
} // namespace tortoise

#endif