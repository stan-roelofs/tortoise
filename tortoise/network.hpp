#ifndef TORTOISE_NETWORKING_HPP
#define TORTOISE_NETWORKING_HPP

#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#endif

namespace tortoise
{
    namespace network
    {
        enum class TransportProtocol
        {
            TCP,
            UDP
        };

        enum class InternetProtocol
        {
            Unknown,
            IPv4,
            IPv6
        };

#define htonll(x) ((1 == htonl(1)) ? (x) : ((uint64_t)htonl((x)&0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1 == ntohl(1)) ? (x) : ((uint64_t)ntohl((x)&0xFFFFFFFF) << 32) | ntohl((x) >> 32))

        inline std::uint16_t HostToNetwork(std::uint16_t value)
        {
            return htons(value);
        }

        inline std::uint32_t HostToNetwork(std::uint32_t value)
        {
            return htonl(value);
        }

        inline std::uint64_t HostToNetwork(std::uint64_t value)
        {
            return htonll(value);
        }

        inline std::uint16_t NetworkToHost(std::uint16_t value)
        {
            return ntohs(value);
        }

        inline std::uint32_t NetworkToHost(std::uint32_t value)
        {
            return ntohl(value);
        }

        inline std::uint64_t NetworkToHost(std::uint64_t value)
        {
            return ntohll(value);
        }

#undef htonll
#undef ntohll
    } // namespace network
} // namespace tortoise

#endif