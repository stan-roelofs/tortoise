#include "bitfield.hpp"

#include <tortoise/exception.hpp>

namespace tortoise
{
	Bitfield::Bitfield(std::size_t nr_pieces) : nr_pieces_(nr_pieces)
	{
		bitfield_.resize((nr_pieces + 7) / 8, 0);
	}

	bool Bitfield::HasPiece(std::uint32_t index) const
	{
		if (index >= nr_pieces_)
			throw InvalidArgumentException("Invalid bitfield index");
		return (bitfield_[index / 8] & (1 << (7 - (index % 8)))) != 0;
	}

	void Bitfield::SetHavePiece(std::uint32_t index)
	{
		if (index >= nr_pieces_)
			throw InvalidArgumentException("Invalid bitfield index");
		bitfield_[index / 8] |= 1 << (7 - (index % 8));
	}

	void Bitfield::FromBytes(ByteVector bytes)
	{
		if (bytes.size() != bitfield_.size())
			throw InvalidArgumentException("Invalid bitfield size");
		bitfield_ = std::move(bytes);
	}

	const ByteVector &Bitfield::AsBytes() const
	{
		return bitfield_;
	}
}