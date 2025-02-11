#ifndef TORTOISE_UTIL_HPP
#define TORTOISE_UTIL_HPP

#include <type_traits>
#include <vector>

namespace tortoise
{
    using ByteVector = std::vector<std::uint8_t>;

    // TODO add tests
    namespace util
    {
        template <typename T>
        inline void Write(ByteVector &buffer, const T &value)
        {
            static_assert(std::is_integral<T>::value, "T must be an integral type.");
            buffer.insert(buffer.end(), reinterpret_cast<const std::uint8_t *>(&value), reinterpret_cast<const std::uint8_t *>(&value) + sizeof(T));
        }

        inline void Write(ByteVector &buffer, const void *data, std::size_t size)
        {
            buffer.insert(buffer.end(), reinterpret_cast<const std::uint8_t *>(data), reinterpret_cast<const std::uint8_t *>(data) + size);
        }

        template <typename T>
        inline T Read(const ByteVector &buffer, std::size_t index)
        {
            if (index + sizeof(T) > buffer.size())
                throw std::out_of_range("Index out of range");
            return *reinterpret_cast<const T *>(&buffer[index]);
        }
    }
}

#endif