#pragma once

#include "base/base.h"
#include "application.h"

struct ft_timer
{
	uint64_t start;
};

FT_INLINE void
ft_timer_reset( struct ft_timer* timer )
{
	timer->start = ft_get_time();
}

FT_INLINE uint64_t
ft_timer_get_ticks( struct ft_timer* timer )
{
	return ft_get_time() - timer->start;
}
