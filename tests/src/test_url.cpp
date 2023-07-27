#include <gtest/gtest.h>

#include "url.hpp"

TEST(URL, parse)
{
    const std::string url = "http://www.example.com:8080/path/to/file";
    const tortoise::URL parsed_url(url);

    EXPECT_EQ(parsed_url.GetProtocol(), "http");
    EXPECT_EQ(parsed_url.GetHost(), "www.example.com");
    EXPECT_EQ(parsed_url.GetPort(), "8080");
    EXPECT_EQ(parsed_url.GetPath(), "/path/to/file");
}

TEST(URL, parse_no_port)
{
    const std::string url = "http://www.example.com/path/to/file";
    const tortoise::URL parsed_url(url);

    EXPECT_EQ(parsed_url.GetProtocol(), "http");
    EXPECT_EQ(parsed_url.GetHost(), "www.example.com");
    EXPECT_EQ(parsed_url.GetPort(), "");
    EXPECT_EQ(parsed_url.GetPath(), "/path/to/file");
}

TEST(URL, parse_no_path)
{
    const std::string url = "http://www.example.com:9000";
    const tortoise::URL parsed_url(url);

    EXPECT_EQ(parsed_url.GetProtocol(), "http");
    EXPECT_EQ(parsed_url.GetHost(), "www.example.com");
    EXPECT_EQ(parsed_url.GetPort(), "9000");
    EXPECT_EQ(parsed_url.GetPath(), "");
}

TEST(URL, no_protocol)
{
    const std::string url = "www.example.com:9000";
    EXPECT_THROW(tortoise::URL x(url), tortoise::URLException);
}

TEST(URL, to_string)
{
    const std::string url = "http://www.example.com:8080/path/to/file";
    const tortoise::URL parsed_url(url);

    EXPECT_EQ(parsed_url.ToString(), url);
}