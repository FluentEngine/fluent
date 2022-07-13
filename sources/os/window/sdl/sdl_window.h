#pragma once

#ifdef FT_WINDOW_SDL
#include "os/window/window.h"

struct ft_window*
sdl_create_window( const struct ft_window_info* info );
#endif
