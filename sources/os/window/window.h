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

FT_API void
ft_window_show_cursor( struct ft_window*, bool show );

FT_API bool
ft_window_should_close( const struct ft_window* );

FT_API void
ft_window_get_vulkan_instance_extensions( const struct ft_window* window,
                                          uint32_t*               count,
                                          const char**            names );

FT_API void
ft_window_create_vulkan_surface( const struct ft_window*             window,
                                 VkInstance                          instance,
                                 const struct VkAllocationCallbacks* allocator,
                                 VkSurfaceKHR*                       surface );

FT_API void
ft_poll_events();

FT_API const uint8_t*
             ft_get_keyboard_state( uint32_t* key_count );

FT_API uint32_t
ft_get_mouse_state( int32_t* x, int32_t* y );

FT_API FT_INLINE uint32_t
ft_window_get_framebuffer_width( const struct ft_window* window )
{
	uint32_t w, h;
	ft_window_get_framebuffer_size( window, &w, &h );
	return w;
}

FT_API FT_INLINE uint32_t
ft_window_get_framebuffer_height( const struct ft_window* window )
{
	uint32_t w, h;
	ft_window_get_framebuffer_size( window, &w, &h );
	return h;
}

FT_API FT_INLINE uint32_t
ft_window_get_width( const struct ft_window* window )
{
	uint32_t w, h;
	ft_window_get_size( window, &w, &h );
	return w;
}

FT_API FT_INLINE uint32_t
ft_window_get_height( const struct ft_window* window )
{
	uint32_t w, h;
	ft_window_get_size( window, &w, &h );
	return h;
}

FT_API FT_INLINE float
ft_window_get_aspect( const struct ft_window* window )
{
	uint32_t w, h;
	ft_window_get_size( window, &w, &h );
	return ( float ) w / ( float ) h;
}