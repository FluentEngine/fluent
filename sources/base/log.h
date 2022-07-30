#pragma once

enum ft_log_level
{
	FT_LOG_LEVEL_TRACE,
	FT_LOG_LEVEL_DEBUG,
	FT_LOG_LEVEL_INFO,
	FT_LOG_LEVEL_WARN,
	FT_LOG_LEVEL_ERROR
};

FT_API bool
ft_log_init( enum ft_log_level log_level );

FT_API void
ft_log_shutdown( void );

FT_API void
ft_log_set_level( enum ft_log_level level );

FT_API void
ft_log( enum ft_log_level level, const char* fmt, ... );

#if FT_DEBUG
#define FT_TRACE( fmt, ... ) ft_log( FT_LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__ )
#define FT_INFO( fmt, ... )  ft_log( FT_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__ )
#define FT_WARN( fmt, ... )  ft_log( FT_LOG_LEVEL_WARN, fmt, ##__VA_ARGS__ )
#define FT_ERROR( fmt, ... ) ft_log( FT_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__ )
#else
#define FT_TRACE( fmt, ... )
#define FT_INFO( fmt, ... )
#define FT_WARN( fmt, ... )
#define FT_ERROR( fmt, ... )
#endif

#ifdef _WIN32
#undef ERROR
#endif
