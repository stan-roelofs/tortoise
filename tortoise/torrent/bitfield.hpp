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
		void SetHavePiece(std::uint32_t index);

		void FromBytes(ByteVector bytes);
		const ByteVector &AsBytes() const;

	private:
		std::size_t nr_pieces_;
		ByteVector bitfield_;
	};
}

#endif