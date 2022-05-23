#pragma once

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include <stdio.h>
#include <string.h>

	enum LogLevel
	{
		FT_TRACE,
		FT_DEBUG,
		FT_INFO,
		FT_WARN,
		FT_ERROR
	};

	void
	log_init( enum LogLevel log_level );

	void
	log_shutdown( void );

	int
	logger_initConsoleLogger( FILE* output );

	void
	logger_setLevel( enum LogLevel level );

	enum LogLevel
	logger_getLevel( void );

	int
	logger_isEnabled( enum LogLevel level );

	void
	logger_autoFlush( long interval );

	void
	logger_flush( void );

	void
	logger_log( enum LogLevel level, const char* fmt, ... );

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#ifdef FLUENT_DEBUG
#define FT_TRACE( fmt, ... ) logger_log( FT_TRACE, fmt, ##__VA_ARGS__ )
#define FT_INFO( fmt, ... )  logger_log( FT_INFO, fmt, ##__VA_ARGS__ )
#define FT_WARN( fmt, ... )  logger_log( FT_WARN, fmt, ##__VA_ARGS__ )
#define FT_ERROR( fmt, ... ) logger_log( FT_ERROR, fmt, ##__VA_ARGS__ )
#else
#define FT_TRACE( fmt, ... )
#define FT_INFO( fmt, ... )
#define FT_WARN( fmt, ... )
#define FT_ERROR( fmt, ... )
#endif

#ifdef _WIN32
#undef ERROR
#endif
