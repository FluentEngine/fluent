#pragma once

#include "base/base.h"

struct ft_timer
{
    uint64_t start;
};

FT_API void
ft_ticks_init();

FT_API void
ft_ticks_shutdown();

FT_API uint64_t
ft_get_ticks();

FT_API void
ft_nanosleep( int64_t milliseconds );

FT_INLINE void
ft_timer_reset( struct ft_timer* timer )
{
	timer->start = ft_get_ticks();
}

FT_INLINE uint64_t
ft_timer_get_ticks( struct ft_timer* timer )
{
	return ft_get_ticks() - timer->start;
}
