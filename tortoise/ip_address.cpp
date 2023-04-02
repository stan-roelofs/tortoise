#include "ip_address.hpp"

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
    IPAddress::IPAddress(const std::string &address)
    {
        const bool is_ipv4 = address.find(':') == std::string::npos;

        if (is_ipv4)
        {
            ipv4_address_t result;
            assert((result.size() * sizeof(result[0])) >= sizeof(in_addr));
            if (inet_pton(AF_INET, address.c_str(), result.data()) != 1)
                throw ParseException("invalid IPv4 address");
            address_ = result;
        }
        else
        {
            ipv6_address_t result;
            assert((result.size() * sizeof(result[0])) >= sizeof(in_addr));
            if (inet_pton(AF_INET6, address.c_str(), result.data()) != 1)
                throw ParseException("invalid IPv6 address");
            address_ = result;
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
        std::stringstream stream;
        if (IsIPv4())
        {
            const auto &address = std::get<0>(address_);
            for (std::size_t i = 0; i < address.size(); ++i)
            {
                if (i > 0)
                    stream << '.';
                stream << static_cast<int>(address[i]);
            }
        }
        else
        {
            const auto &address = std::get<1>(address_);
            for (std::size_t i = 0; i < address.size(); ++i)
            {
                if (i > 0)
                    stream << ':';
                stream << std::hex << htons(address[i]);
            }
        }
        return stream.str();
    }

    std::vector<std::uint8_t> IPAddress::ToVector() const
    {
        std::vector<std::uint8_t> result;

        if (IsIPv4())
        {
            const auto& address = std::get<0>(address_);
			std::copy(address.begin(), address.end(), std::back_inserter(result));
        }
        else {
			const auto& address = std::get<1>(address_);
			std::copy(address.begin(), address.end(), std::back_inserter(result));                
        }
        
        return result;
    }
} // namespace tortoise