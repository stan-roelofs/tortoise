#include <tortoise/sha1_hash.hpp>

namespace tortoise
{
    SHA1Hash::SHA1Hash(const std::array<uint8_t, 20> &bytes) : bytes_(bytes)
    {
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