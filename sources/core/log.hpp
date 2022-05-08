#pragma once

#include <logger.h>

#ifdef FLUENT_DEBUG
#define FT_TRACE( fmt, ... ) CLOG_TRACE( fmt, ##__VA_ARGS__ )
#define FT_INFO( fmt, ... )  CLOG_INFO( fmt, ##__VA_ARGS__ )
#define FT_WARN( fmt, ... )  CLOG_WARN( fmt, ##__VA_ARGS__ )
#define FT_ERROR( fmt, ... ) CLOG_ERROR( fmt, ##__VA_ARGS__ )
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
	TRACE,
	DEBUG,
	INFO,
	WARN,
	ERROR
};
} // namespace fluent
