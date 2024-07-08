#include <gtest/gtest.h>

#include "network/ip_address.hpp"

TEST(IPAddress, parse_ipv4_address)
{
    auto address = tortoise::network::IPAddress::FromString("0.0.0.0");
    ASSERT_NE(address, std::nullopt);
    EXPECT_TRUE(address->IsIPv4());
    EXPECT_EQ("0.0.0.0", address->ToString());
}

TEST(IPAddress, parse_ipv4_address_incomplete)
{
    EXPECT_EQ(std::nullopt, tortoise::network::IPAddress::FromString("0.0"));
}

TEST(IPAddress, parse_ipv4_address_invalid)
{
    EXPECT_EQ(std::nullopt, tortoise::network::IPAddress::FromString("0.0.0.0.0"));
}

TEST(IPAddress, parse_ipv4_address_invalid_character)
{
    EXPECT_EQ(std::nullopt, tortoise::network::IPAddress::FromString("0.0.0.0a"));
}

TEST(IPAddress, parse_ipv4_address_out_of_range)
{
    EXPECT_EQ(std::nullopt, tortoise::network::IPAddress::FromString("0.0.0.256"));
}

TEST(IPAddress, parse_ipv6_address_reduced)
{
    auto address = tortoise::network::IPAddress::FromString("0::0");
    ASSERT_NE(address, std::nullopt);
    EXPECT_TRUE(address->IsIPv6());
    EXPECT_EQ("::", address->ToString());
}

TEST(IPAddress, parse_ipv6_address_full)
{
    auto address = tortoise::network::IPAddress::FromString("2001:0db8:85a3:0000:0000:8a2e:0370:7334");
    ASSERT_NE(address, std::nullopt);
    EXPECT_TRUE(address->IsIPv6());
    EXPECT_EQ("2001:db8:85a3::8a2e:370:7334", address->ToString());
}

TEST(IPAddress, parse_ipv6_address_multiple_reduced)
{
    EXPECT_EQ(std::nullopt, tortoise::network::IPAddress::FromString("11::22::33"));
}