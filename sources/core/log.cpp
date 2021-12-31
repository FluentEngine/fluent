#include "core/log.hpp"

namespace fluent
{

spdlog::level::level_enum util_to_spdlog_level(LogLevel log_level)
{
    switch (log_level)
    {
    case LogLevel::eOff:
        return spdlog::level::off;
    case LogLevel::eTrace:
        return spdlog::level::trace;
    case LogLevel::eDebug:
        return spdlog::level::debug;
    case LogLevel::eInfo:
        return spdlog::level::info;
    case LogLevel::eWarn:
        return spdlog::level::warn;
    case LogLevel::eError:
        return spdlog::level::err;
    default:
        return spdlog::level::off;
    }
}

} // namespace fluent