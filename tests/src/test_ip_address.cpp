#include <gtest/gtest.h>

#include "../tortoise/ip_address.hpp"

TEST(IPAddress, parse_ipv4_address)
{
    tortoise::IPAddress address("0.0.0.0");
    EXPECT_TRUE(address.IsIPv4());
    EXPECT_EQ("0.0.0.0", address.ToString());
}

TEST(IPAddress, parse_ipv4_address_incomplete_throws)
{
    EXPECT_THROW(tortoise::IPAddress address("0.0.0"), tortoise::IPAddress::ParseException);
}

TEST(IPAddress, parse_ipv4_address_invalid_throws)
{
    EXPECT_THROW(tortoise::IPAddress address("0..0.0"), tortoise::IPAddress::ParseException);
}

TEST(IPAddress, parse_ipv4_address_invalid_character_throws)
{
    EXPECT_THROW(tortoise::IPAddress address("0.abcdef.0.0"), tortoise::IPAddress::ParseException);
}

TEST(IPAddress, parse_ipv4_address_out_of_range_throws)
{
    EXPECT_THROW(tortoise::IPAddress address("0.256.0.0"), tortoise::IPAddress::ParseException);
}

TEST(IPAddress, parse_ipv6_address_reduced)
{
    tortoise::IPAddress address("0::0");
    EXPECT_TRUE(address.IsIPv6());
    EXPECT_EQ("::", address.ToString());
}

TEST(IPAddress, parse_ipv6_address_full)
{
    tortoise::IPAddress address("2001:0db8:85a3:0000:0000:8a2e:0370:7334");
    EXPECT_TRUE(address.IsIPv6());
    EXPECT_EQ("2001:db8:85a3::8a2e:370:7334", address.ToString());
}

TEST(IPAddress, parse_ipv6_address_multiple_reduced_throws)
{
    EXPECT_THROW(tortoise::IPAddress address("11::22::33"), tortoise::IPAddress::ParseException);
}