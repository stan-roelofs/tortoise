#ifndef TORTOISE_SHA1_HASH_HPP
#define TORTOISE_SHA1_HASH_HPP

#include <array>

namespace tortoise
{
    class SHA1Hash
    {
    public:
        SHA1Hash(const std::array<uint8_t, 20> &bytes);

        bool operator==(const SHA1Hash &other) const;
        bool operator!=(const SHA1Hash &other) const;

        const std::array<uint8_t, 20> &GetBytes() const;

    private:
        std::array<uint8_t, 20> bytes_;
    };
} // namespace tortoise

#endif