#pragma once

#include <spdlog/spdlog.h>

#ifdef FLUENT_DEBUG
#    define FT_TRACE( ... ) spdlog::trace( __VA_ARGS__ )
#    define FT_INFO( ... )  spdlog::info( __VA_ARGS__ )
#    define FT_WARN( ... )  spdlog::warn( __VA_ARGS__ )
#    define FT_ERROR( ... ) spdlog::error( __VA_ARGS__ )
#else
#    define FT_TRACE( ... )
#    define FT_INFO( ... )
#    define FT_WARN( ... )
#    define FT_ERROR( ... )
#endif

namespace fluent
{
enum class LogLevel
{
    eOff,
    eTrace,
    eDebug,
    eInfo,
    eWarn,
    eError
};

spdlog::level::level_enum to_spdlog_level( LogLevel log_level );

} // namespace fluent
