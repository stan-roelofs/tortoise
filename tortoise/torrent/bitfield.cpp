#include "bitfield.hpp"

#include <tortoise/exceptions.hpp>

namespace tortoise
{
	Bitfield::Bitfield(std::size_t nr_pieces)
	{
		bitfield_.resize(nr_pieces, false);
	}

	bool Bitfield::HasPiece(std::uint32_t index) const
	{
		return bitfield_[index];
	}

	void Bitfield::SetHavePiece(std::uint32_t index)
	{
		bitfield_[index] = true;
	}

	void Bitfield::SetBitfield(const ByteVector& bitfield)
	{
		if (bitfield.size() != (bitfield_.size() + 7) / 8)
			throw InvalidArgumentException("Invalid bitfield size");

		for (std::size_t byte = 0; byte < bitfield.size(); ++byte)
		{
			const uint8_t byte_value = bitfield[byte];
			const std::size_t bit_offset = static_cast<std::size_t>(byte) * 8;
			for (uint8_t bit = 0; bit < 8 && bit_offset + bit < bitfield_.size(); ++bit)
			{
				bitfield_[bit_offset + bit] = (byte_value & (1 << (7 - bit))) != 0;
			}
		}
	}
}