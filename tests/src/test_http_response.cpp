#include <gtest/gtest.h>

#include "../tortoise/http/exception.hpp"
#include "../tortoise/http/response.hpp"

TEST(http_response, test_success)
{
    const std::string response = "HTTP/1.1 200 OK\r\n\r\n";
    const tortoise::http::Response http_response(response);
    EXPECT_TRUE(http_response.Success());
}

TEST(http_response, test_failure)
{
    const std::string response = "HTTP/1.1 404 Not Found\r\n\r\n";
    const tortoise::http::Response http_response(response);
    EXPECT_FALSE(http_response.Success());
}

TEST(http_response, test_http_version)
{
    const std::string response = "HTTP/1.1 200 OK\r\n\r\n";
    const tortoise::http::Response http_response(response);
    EXPECT_EQ(http_response.GetHTTPVersion(), "HTTP/1.1");
}

TEST(http_response, test_status_code)
{
    const std::string response = "HTTP/1.1 200 OK\r\n\r\n";
    const tortoise::http::Response http_response(response);
    EXPECT_EQ(http_response.GetStatusCode(), 200);
}

TEST(http_response, test_status_text)
{
    const std::string response = "HTTP/1.1 200 OK\r\n\r\n";
    const tortoise::http::Response http_response(response);
    EXPECT_EQ(http_response.GetStatusText(), "OK");
}

TEST(http_response, test_headers)
{
    const std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    const tortoise::http::Response http_response(response);
    EXPECT_EQ(http_response.GetHeaders(), "Content-Type: text/html");
}

TEST(http_response, test_body)
{
    const std::string response = "HTTP/1.1 200 OK\r\n\r\n<html><body><h1>Hello, World!</h1></body></html>";
    const tortoise::http::Response http_response(response);
    EXPECT_EQ(http_response.GetBody(), "<html><body><h1>Hello, World!</h1></body></html>");
}

TEST(http_response, empty_throws)
{
    EXPECT_THROW(tortoise::http::Response(""), tortoise::http::Exception);
}

TEST(http_response, missing_version_throws)
{
    EXPECT_THROW(tortoise::http::Response("200 OK\r\n\r\n"), tortoise::http::Exception);
}

TEST(http_response, missing_status_code_throws)
{
    EXPECT_THROW(tortoise::http::Response("HTTP/1.1 OK\r\n\r\n"), tortoise::http::Exception);
}

TEST(http_response, missing_status_text_throws)
{
    EXPECT_THROW(tortoise::http::Response("HTTP/1.1 200\r\n\r\n"), tortoise::http::Exception);
}