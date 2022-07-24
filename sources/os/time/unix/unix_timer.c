#include "os/time/timer.h"
#if FT_PLATFORM_UNIX
#include <time.h>

struct
{
	struct timespec start;
} linux_ticks;

FT_API void
ft_ticks_init()
{
	clock_gettime( CLOCK_MONOTONIC_RAW, &linux_ticks.start );
}

FT_API void
ft_ticks_shutdown()
{
}

FT_API uint64_t
ft_get_ticks()
{
	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC_RAW, &now );
	return (
	    uint64_t ) ( ( ( int64_t ) ( now.tv_sec - linux_ticks.start.tv_sec ) *
	                   1000 ) +
	                 ( ( now.tv_nsec - linux_ticks.start.tv_sec ) /
	                   CLOCKS_PER_SEC ) );
}

FT_API void
ft_nanosleep( int64_t milliseconds )
{
	nanosleep( ( const struct timespec[] ) { { 0, milliseconds } }, NULL );
}

#endif
