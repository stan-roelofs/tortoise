#ifndef TORTOISE_IP_ADDRESS_HPP
#define TORTOISE_IP_ADDRESS_HPP

#include <array>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <tortoise/exception.hpp>

namespace tortoise
{
    namespace network
    {
        class IPAddress
        {
        public:
            using ipv4_address_t = std::array<std::uint8_t, 4>;
            using ipv6_address_t = std::array<std::uint8_t, 16>;

            /*! \brief Creates an instance of IPAddress from a string.
             *  \param address A dotted quad or rfc3513 hexed formatted string.
             *  \returns An IPAddress object if the string is a valid IP address, or an empty optional if it is not.
             */
            static std::optional<IPAddress> FromString(const std::string &address);

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

            //! \returns the IP address as a vector of bytes. For IPv4 the length will be 4, for IPv6 the length will be 16.
            std::vector<std::uint8_t> ToVector() const;

        private:
            std::variant<ipv4_address_t, ipv6_address_t> address_;
        };
    }
}

#endif