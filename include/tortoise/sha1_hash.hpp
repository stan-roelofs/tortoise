#ifndef TORTOISE_SHA1_HASH_HPP
#define TORTOISE_SHA1_HASH_HPP

#include <array>
#include <cstdint>

namespace tortoise
{
    class SHA1Hash
    {
    public:
		// This creates a hash with all bytes set to 0.
        SHA1Hash();
        SHA1Hash(const std::array<std::uint8_t, 20> &bytes);

        bool operator==(const SHA1Hash &other) const;
        bool operator!=(const SHA1Hash &other) const;

        const std::array<uint8_t, 20> &GetBytes() const;

    private:
        std::array<uint8_t, 20> bytes_;
    };
} // namespace tortoise

#endif