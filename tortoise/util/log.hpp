#ifndef TORTOISE_LOG_HPP
#define TORTOISE_LOG_HPP

#include <tortoise/logging.hpp>

#include <format>
namespace tortoise
{
	namespace logging
	{
		void LogMessage(Level level, std::string_view tag, std::string message);
	} // namespace logging
} // namespace tortoise

#define LOG_WARN(tag, msg) tortoise::logging::LogMessage(tortoise::logging::Level::Warning, tag, msg)
#define LOG_INFO(tag, msg) tortoise::logging::LogMessage(tortoise::logging::Level::Info, tag, msg)
#define LOG_ERROR(tag, msg) tortoise::logging::LogMessage(tortoise::logging::Level::Error, tag, msg)
#define LOG_DEBUG(tag, msg) tortoise::logging::LogMessage(tortoise::logging::Level::Debug, tag, msg)

#endif