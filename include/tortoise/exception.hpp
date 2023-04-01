#ifndef TORTOISE_EXCEPTION_HPP
#define TORTOISE_EXCEPTION_HPP

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
} // namespace tortoise

#endif