#pragma once

#include <time.h>

struct Timer
{
	struct timespec start;
};

static inline void
timer_reset( struct Timer* timer )
{
	clock_gettime( CLOCK_MONOTONIC_RAW, &timer->start );
}

static inline u64
timer_get_ticks( struct Timer* timer )
{
	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC_RAW, &now );
	return (
	    u64 ) ( ( ( i64 ) ( now.tv_sec - timer->start.tv_sec ) * 1000 ) +
	            ( ( now.tv_nsec - timer->start.tv_nsec ) / CLOCKS_PER_SEC ) );
}
