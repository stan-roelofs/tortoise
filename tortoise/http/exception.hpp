#ifndef TORTOISE_HTTP_HPP
#define TORTOISE_HTTP_HPP

#include <tortoise/exceptions.hpp>

namespace tortoise
{
    namespace http
    {
        class Exception : public tortoise::Exception
        {
        public:
            Exception(const std::string &msg) : tortoise::Exception(msg) {}
        };
    } // namespace http
}

#endif