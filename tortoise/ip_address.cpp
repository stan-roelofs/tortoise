#include <tortoise/ip_address.hpp>

#include <cassert>
#include <limits>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace tortoise
{
    std::optional<IPAddress> IPAddress::FromString(const std::string &address)
    {
        const bool is_ipv4 = address.find(':') == std::string::npos;

        if (is_ipv4)
        {
            ipv4_address_t result;
            assert((result.size() * sizeof(result[0])) >= sizeof(in_addr));
            if (inet_pton(AF_INET, address.c_str(), result.data()) != 1)
                return {};

            return IPAddress(result);
        }
        else
        {
            ipv6_address_t result;
            assert((result.size() * sizeof(result[0])) >= sizeof(in_addr));
            if (inet_pton(AF_INET6, address.c_str(), result.data()) != 1)
                return {};
            return IPAddress(result);
        }
    }

    IPAddress::IPAddress(const ipv4_address_t &address)
    {
        address_ = address;
    }

    IPAddress::IPAddress(const ipv6_address_t &address)
    {
        address_ = address;
    }

    bool IPAddress::IsIPv4() const
    {
        return address_.index() == 0;
    }

    bool IPAddress::IsIPv6() const
    {
        return address_.index() == 1;
    }

    std::string IPAddress::ToString() const
    {
        if (IsIPv4())
        {
            char buffer[INET_ADDRSTRLEN];
            const auto &address = std::get<0>(address_);
            if (inet_ntop(AF_INET, address.data(), buffer, INET_ADDRSTRLEN) == nullptr)
                return "";
            return std::string(buffer);
        }
        else
        {
            const auto &address = std::get<1>(address_);
            char buffer[INET6_ADDRSTRLEN];
            if (inet_ntop(AF_INET6, address.data(), buffer, INET6_ADDRSTRLEN) == nullptr)
                return "";
            return std::string(buffer);
        }
    }

    std::vector<std::uint8_t> IPAddress::ToVector() const
    {
        std::vector<std::uint8_t> result;

        if (IsIPv4())
        {
            const auto &address = std::get<0>(address_);
            std::copy(address.begin(), address.end(), std::back_inserter(result));
        }
        else
        {
            const auto &address = std::get<1>(address_);
            std::copy(address.begin(), address.end(), std::back_inserter(result));
        }

        return result;
    }
} // namespace tortoise