#ifndef TORTOISE_PIECE_MANAGER_HPP
#define TORTOISE_PIECE_MANAGER_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
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

		bool operator<(const Block &other) const
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

		class Peer
		{
		public:
			Peer(PieceManager &manager) : piece_manager_(manager)
			{
				piece_manager_.RegisterPeer(this);
			}
			virtual ~Peer()
			{
				piece_manager_.UnregisterPeer(this);
			}
			virtual void SendHave(std::uint32_t piece_index) = 0;

		protected:
			PieceManager &piece_manager_;
		};

		PieceManager(std::shared_ptr<const Metainfo> metainfo);
		~PieceManager();

		bool HaveInterestingPiece(Peer *peer);

		const Bitfield &GetBitfield() const;

		void RegisterPeer(Peer *peer);
		void UnregisterPeer(Peer *peer);

		void SetPeerBitfield(Peer *peer, Bitfield bitfield);
		void SetPeerHave(Peer *peer, std::uint32_t piece);
		std::vector<Block> GetRequests(Peer *peer);
		void ReceiveBlock(Peer *peer, std::uint32_t piece_index, std::uint32_t offset, ByteVector block);

	private:
		struct BlockData
		{
			Block block;
			ByteVector data;
		};
		struct Piece
		{
			Piece(std::uint32_t index, std::uint32_t length) : index(index), length(length), done(false), requested(false)
			{
			}
			std::uint32_t index;
			std::uint32_t length;
			std::vector<BlockData> blocks;
			bool done;
			bool requested;
		};
		struct PeerData
		{
			Bitfield bitfield;
			std::vector<uint32_t> piece_queue;
		};

		std::vector<Piece> pieces_;
		std::map<Peer *, PeerData> peers_;

		std::shared_ptr<const Metainfo> metainfo_;
		Bitfield bitfield_;
	};
} // namespace tortoise

#endif