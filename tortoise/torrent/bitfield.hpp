#ifndef TORTOISE_TORRENT_BITFIELD_HPP
#define TORTOISE_TORRENT_BITFIELD_HPP

#include "../util/util.hpp"

namespace tortoise
{
	class Bitfield
	{
	public:
		Bitfield(std::size_t nr_pieces);

		bool HasPiece(std::uint32_t index) const;
		void SetBitfield(const ByteVector& bitfield);
		void SetHavePiece(std::uint32_t index);

	private:
		std::vector<bool> bitfield_;
	};
}

#endif