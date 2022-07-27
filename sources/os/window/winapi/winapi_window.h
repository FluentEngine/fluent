#pragma once

#include "base/base.h"

#if FT_PLATFORM_WINDOWS

#include "os/window/window.h"

struct ft_window*
winapi_create_window( const struct ft_window_info* info );

#endif
