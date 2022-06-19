#pragma once

#include <time.h>
#include "base/base.h"

struct ft_timer
{
	struct timespec start;
};

FT_INLINE void
ft_timer_reset( struct ft_timer* timer )
{
	clock_gettime( CLOCK_MONOTONIC_RAW, &timer->start );
}

FT_INLINE uint64_t
ft_timer_get_ticks( struct ft_timer* timer )
{
	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC_RAW, &now );
	return ( uint64_t ) ( ( ( int64_t ) ( now.tv_sec - timer->start.tv_sec ) *
	                        1000 ) +
	                      ( ( now.tv_nsec - timer->start.tv_nsec ) /
	                        CLOCKS_PER_SEC ) );
}
