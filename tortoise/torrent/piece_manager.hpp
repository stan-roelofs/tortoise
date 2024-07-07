#ifndef TORTOISE_PIECE_MANAGER_HPP
#define TORTOISE_PIECE_MANAGER_HPP

#include <cstdint>
#include <mutex>
#include <vector>

#include <tortoise/metainfo.hpp>

namespace tortoise
{
    struct PieceBlock
    {
        std::uint32_t piece_index;
        std::uint32_t begin;
        std::uint32_t length;
    };

    class PieceManager
    {
    public:
        static constexpr std::uint32_t BLOCK_SIZE = 2 ^ 14; // 2^14 is used by near all clients and some even enforce this size.

        PieceManager(std::size_t piece_count);
        ~PieceManager();

        bool Needed(const PieceBlock &block);

        static std::uint32_t NumberOfBlocksInPiece(const Metainfo &metainfo, std::uint32_t piece_index);
        static std::uint32_t BlockLength(const Metainfo &metainfo, std::uint32_t piece_index, std::uint32_t block_index);

    private:
        struct Piece
        {
            std::vector<PieceBlock> blocks;
        };

        std::vector<Piece> pieces_;
        std::mutex mutex_;
    };
} // namespace tortoise

#endif