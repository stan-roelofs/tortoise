#ifndef TORTOISE_HASH_HPP
#define TORTOISE_HASH_HPP

#include <array>
#include <cstdint>

#include <tortoise/exception.hpp>

namespace tortoise
{
    namespace hash
    {
        class Exception : public tortoise::Exception
        {
        public:
            Exception(const std::string &message) : tortoise::Exception(message)
            {
            }
        };

        /*! \brief Creates a SHA1 hash from a buffer.
         *  \param buffer The buffer to hash.
         *  \param size The size of the buffer.
         *  \returns The SHA1 hash of the buffer.
         *  \throws tortoise::hash::Exception if the hash could not be created.
         */
        std::array<std::uint8_t, 20> CreateSHA1(const void *buffer, std::size_t size);
    }
}

#endif
