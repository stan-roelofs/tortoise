#ifndef TORTOISE_EXCEPTIONS_HPP
#define TORTOISE_EXCEPTIONS_HPP

#include "exception.hpp"

namespace tortoise
{
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