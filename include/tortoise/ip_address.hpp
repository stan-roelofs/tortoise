#ifndef TORTOISE_IP_ADDRESS_HPP
#define TORTOISE_IP_ADDRESS_HPP

#include <array>
#include <string>
#include <variant>

#include "exceptions.hpp"

namespace tortoise
{
    class IPAddress
    {
    public:
        class ParseException : public Exception
        {
        public:
            ParseException(const std::string &message) : Exception(message) {}
        };

        using ipv4_address_t = std::array<std::uint8_t, 4>;
        using ipv6_address_t = std::array<std::uint16_t, 8>;

        /*! \brief Creates an instance of IPAddress from a dotted quad or rfc3513 hexed formatted string.
         *  \throws ParseException if the string is not a valid IP address.
         */
        IPAddress(const std::string &address);

        //! \brief Creates an instance of IPAddress from an IPv4 address.
        IPAddress(const ipv4_address_t &address);

        //! \brief Creates an instance of IPAddress from an IPv6 address.
        IPAddress(const ipv6_address_t &address);

        //! \returns true if the IP address is an IPv4 address.
        bool IsIPv4() const;

        //! \returns true if the IP address is an IPv6 address.
        bool IsIPv6() const;

        //! \returns the IP address as a string. If the IP address is an IPv4 address, it will be returned in dotted quad format. If the IP address is an IPv6 address, it will be returned in rfc3513 hexed format.
        std::string ToString() const;

    private:
        std::variant<ipv4_address_t, ipv6_address_t> address_;
    };
}

#endif