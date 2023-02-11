#ifndef TORTOISE_COMMUNICATION_HPP
#define TORTOISE_COMMUNICATION_HPP

#include <array>
#include <cstdint>

#include "version.hpp"

namespace tortoise
{
    // TODO move to cpp
    inline std::array<uint8_t, 20> GeneratePeerID()
    {
        std::array<uint8_t, 20> peer_id;
        peer_id.at(0) = '-';
        peer_id.at(1) = 'T';
        peer_id.at(2) = 'R';
        static_assert(TORTOISE_VERSION_MAJOR < 10, "TORTOISE_VERSION_MAJOR must be less than 10");
        static_assert(TORTOISE_VERSION_MINOR < 10, "TORTOISE_VERSION_MINOR must be less than 10");
        static_assert(TORTOISE_VERSION_PATCH < 100, "TORTOISE_VERSION_PATCH must be less than 100");
        peer_id.at(3) = TORTOISE_VERSION_MAJOR;
        peer_id.at(4) = TORTOISE_VERSION_MINOR;
        peer_id.at(5) = TORTOISE_VERSION_PATCH / 10;
        peer_id.at(6) = TORTOISE_VERSION_PATCH % 10;
        peer_id.at(7) = '-';
        for (size_t i = 8; i < peer_id.size(); i++)
            peer_id.at(i) = rand() % 256;

        return peer_id;
    }
} // namespace tortoise

#endif