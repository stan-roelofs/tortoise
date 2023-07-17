#ifndef TORTOISE_PIECE_MANAGER_HPP
#define TORTOISE_PIECE_MANAGER_HPP

#include <cstdint>
#include <mutex>
#include <vector>

namespace tortoise
{
    class PieceManager
    {
    public:
        PieceManager(std::size_t piece_count);
        ~PieceManager();

    private:
        struct Piece
        {
            bool requested = false;
        };

        std::vector<Piece> pieces_;
        std::mutex mutex_;
    };
} // namespace tortoise

#endif