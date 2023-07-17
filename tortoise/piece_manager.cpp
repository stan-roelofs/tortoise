#include "piece_manager.hpp"

namespace tortoise
{
    PieceManager::PieceManager(std::size_t piece_count)
    {
        pieces_.resize(piece_count);
    }

    PieceManager::~PieceManager() = default;
} // namespace tortoise