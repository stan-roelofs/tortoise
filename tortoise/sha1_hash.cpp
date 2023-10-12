#include <tortoise/sha1_hash.hpp>

#include <algorithm>

namespace tortoise
{
    SHA1Hash::SHA1Hash() : bytes_{0}
    {
    }

    SHA1Hash::SHA1Hash(const std::array<uint8_t, 20> &bytes) : bytes_(bytes)
    {
    }

    SHA1Hash::SHA1Hash(std::uint8_t *buffer)
    {
        std::copy_n(buffer, 20, bytes_.begin());
    }

    bool SHA1Hash::operator==(const SHA1Hash &other) const
    {
        return bytes_ == other.bytes_;
    }

    bool SHA1Hash::operator!=(const SHA1Hash &other) const
    {
        return !(*this == other);
    }

    const std::array<uint8_t, 20> &SHA1Hash::GetBytes() const
    {
        return bytes_;
    }
}