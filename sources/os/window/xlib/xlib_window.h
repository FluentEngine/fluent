#pragma once

#include "base/base.h"

#if FT_PLATFORM_LINUX

struct ft_window_info;

struct ft_window*
xlib_create_window( const struct ft_window_info* info );

#endif