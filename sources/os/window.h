#pragma once

#include "base/base.h"
#include "renderer/backend/renderer_enums.h"

struct ft_window_info
{
	const char*          title;
	uint32_t             x;
	uint32_t             y;
	uint32_t             width;
	uint32_t             height;
	bool                 resizable;
	bool                 centered;
	bool                 fullscreen;
	bool                 grab_mouse;
	enum ft_renderer_api renderer_api;
};

struct ft_window;

FT_API struct ft_window*
ft_create_window( const struct ft_window_info* info );

FT_API void
ft_destroy_window( struct ft_window* window );

FT_API void
ft_window_get_size( const struct ft_window* window,
                    uint32_t*               width,
                    uint32_t*               height );

FT_API void
ft_window_get_framebuffer_size( const struct ft_window* window,
                                uint32_t*               width,
                                uint32_t*               height );

FT_API uint32_t
ft_window_get_framebuffer_width( const struct ft_window* window );

FT_API uint32_t
ft_window_get_framebuffer_height( const struct ft_window* window );

FT_API uint32_t
ft_window_get_width( const struct ft_window* window );

FT_API uint32_t
ft_window_get_height( const struct ft_window* window );
FT_API float
ft_window_get_aspect( const struct ft_window* window );
FT_API void
ft_window_show_cursor( bool show );
