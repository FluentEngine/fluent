#pragma once

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include <stdio.h>
#include <string.h>

#if defined( _WIN32 ) || defined( _WIN64 )
#define __FILENAME__                                                           \
	( strrchr( __FILE__, '\\' ) ? strrchr( __FILE__, '\\' ) + 1 : __FILE__ )
#else
#define __FILENAME__                                                           \
	( strrchr( __FILE__, '/' ) ? strrchr( __FILE__, '/' ) + 1 : __FILE__ )
#endif /* defined(_WIN32) || defined(_WIN64) */

	typedef enum
	{
		FT_TRACE,
		FT_DEBUG,
		FT_INFO,
		FT_WARN,
		FT_ERROR
	} LogLevel;

	void
	log_init( LogLevel log_level );

	void
	log_shutdown();

	int
	logger_initConsoleLogger( FILE* output );

	void
	logger_setLevel( LogLevel level );

	LogLevel
	logger_getLevel( void );

	int
	logger_isEnabled( LogLevel level );

	void
	logger_autoFlush( long interval );

	void
	logger_flush( void );

	void
	logger_log( LogLevel    level,
	            const char* file,
	            int         line,
	            const char* fmt,
	            ... );

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#ifdef FLUENT_DEBUG
#define FT_TRACE( fmt, ... )                                                   \
	logger_log( FT_TRACE, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__ )
#define FT_INFO( fmt, ... )                                                    \
	logger_log( FT_INFO, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__ )
#define FT_WARN( fmt, ... )                                                    \
	logger_log( FT_WARN, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__ )
#define FT_ERROR( fmt, ... )                                                   \
	logger_log( FT_ERROR, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__ )
#else
#define FT_TRACE( fmt, ... )
#define FT_INFO( fmt, ... )
#define FT_WARN( fmt, ... )
#define FT_ERROR( fmt, ... )
#endif

#ifdef _WIN32
#undef ERROR
#endif
