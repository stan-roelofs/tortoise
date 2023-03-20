#ifndef TORTOISE_EXCEPTIONS_HPP
#define TORTOISE_EXCEPTIONS_HPP

#include <exception>
#include <string>

namespace tortoise
{
    class Exception : public std::exception
    {
    public:
        Exception(const std::string &msg) : msg_(msg) {}
        virtual ~Exception() = default;
        const char *what() const noexcept override { return msg_.c_str(); }

    private:
        std::string msg_;
    };

    class BencodeException : public Exception
    {
    public:
        BencodeException(const std::string &msg) : Exception(msg) {}
    };

    class ConnectionException : public Exception
    {
    public:
        ConnectionException(const std::string &msg) : Exception(msg) {}
    };

    class SocketException : public Exception
    {
    public:
        SocketException(const std::string &msg) : Exception(msg) {}
    };

    class TrackerException : public Exception
    {
    public:
        TrackerException(const std::string &msg) : Exception(msg) {}
    };

    class URLException : public Exception
    {
    public:
        URLException(const std::string &msg) : Exception(msg) {}
    };

    class TorrentException : public Exception
    {
    public:
        TorrentException(const std::string &msg) : Exception(msg) {}
    };

    class InvalidArgumentException : public Exception
    {
    public:
        InvalidArgumentException(const std::string &msg) : Exception(msg) {}
    };

    class UnsupportedProtocolException : public Exception
    {
    public:
        UnsupportedProtocolException(const std::string &msg) : Exception(msg) {}
    };
} // namespace tortoise

#endif