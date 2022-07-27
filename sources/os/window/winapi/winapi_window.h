#pragma once

#include "base/base.h"

#if FT_PLATFORM_WINDOWS

struct ft_window;

struct ft_window*
winapi_create_window( const struct ft_window_info* info );

#endif
