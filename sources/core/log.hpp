#pragma once

#include <logger.h>

#ifdef FLUENT_DEBUG
#define FT_TRACE( fmt, ... )                                                   \
	logger_log( LogLevel_TRACE, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__ )
#define FT_INFO( fmt, ... )                                                    \
	logger_log( LogLevel_INFO, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__ )
#define FT_WARN( fmt, ... )                                                    \
	logger_log( LogLevel_WARN, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__ )
#define FT_ERROR( fmt, ... )                                                   \
	logger_log( LogLevel_ERROR, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__ )
#else
#define FT_TRACE( fmt, ... )
#define FT_INFO( fmt, ... )
#define FT_WARN( fmt, ... )
#define FT_ERROR( fmt, ... )
#endif

namespace fluent
{
enum class LogLevel
{
	eTrace,
	eDebug,
	eInfo,
	eWarn,
	eError
};
} // namespace fluent
