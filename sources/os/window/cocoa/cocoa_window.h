#pragma once

#ifdef __APPLE__
#include "os/window/window.h"

struct ft_window*
cocoa_create_window( const struct ft_window_info* info );
#endif
