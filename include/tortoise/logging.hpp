#ifndef TORTOISE_TORTOISE_HPP
#define TORTOISE_TORTOISE_HPP

#include <chrono>
#include <functional>
#include <string>

namespace tortoise
{
    namespace logging
    {
        enum class Level
        {
            Debug,
            Info,
            Warning,
            Error,
        };

        struct Message
        {
            std::chrono::system_clock::time_point time;
            Level level;
            std::string tag;
            std::string message;
        };

        using LogReceiver = std::function<void(const Message &)>;
        void RegisterReceiver(LogReceiver receiver);
    }
} // namespace tortoise

#endif