#pragma once

#include "base/base.h"

#include <stdio.h>
#include <string.h>

enum ft_log_level
{
	FT_TRACE,
	FT_DEBUG,
	FT_INFO,
	FT_WARN,
	FT_ERROR
};

FT_API void
ft_log_init( enum ft_log_level log_level );

FT_API void
ft_log_shutdown( void );

FT_API void
ft_log_set_level( enum ft_log_level level );

FT_API void
ft_log( enum ft_log_level level, const char* fmt, ... );

#ifdef FLUENT_DEBUG
#define FT_TRACE( fmt, ... ) ft_log( FT_TRACE, fmt, ##__VA_ARGS__ )
#define FT_INFO( fmt, ... )  ft_log( FT_INFO, fmt, ##__VA_ARGS__ )
#define FT_WARN( fmt, ... )  ft_log( FT_WARN, fmt, ##__VA_ARGS__ )
#define FT_ERROR( fmt, ... ) ft_log( FT_ERROR, fmt, ##__VA_ARGS__ )
#else
#define FT_TRACE( fmt, ... )
#define FT_INFO( fmt, ... )
#define FT_WARN( fmt, ... )
#define FT_ERROR( fmt, ... )
#endif

#ifdef _WIN32
#undef ERROR
#endif
