#pragma once

#include "base/base.h"
#include "renderer/backend/renderer_enums.h"

enum ft_event_type
{
	FT_EVENT_TYPE_QUIT,
	FT_EVENT_TYPE_WINDOW,
	FT_EVENT_TYPE_MOUSEWHEEL,
};

enum ft_window_event_type
{
	FT_WINDOW_EVENT_TYPE_RESIZE,
};

struct ft_window_event
{
	enum ft_window_event_type type;
};

enum ft_mouse_wheel_direction
{
	FT_MOUSE_WHEEL_DIRECTION_NORMAL,
	FT_MOUSE_WHEEL_DIRECTION_FLIPPED,
};

struct ft_mouse_wheel_event
{
	enum ft_mouse_wheel_direction direction;
	uint32_t                      y;
};

struct ft_event
{
	enum ft_event_type          type;
	struct ft_window_event      window;
	struct ft_mouse_wheel_event wheel;
};

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
ft_window_show_cursor( bool show );

FT_API int
ft_window_poll_event( struct ft_event* event );

FT_API void
ft_get_mouse_offset( int32_t* x, int32_t* y );

FT_API const uint8_t*
ft_get_keyboard_state( uint32_t* key_count );

FT_API uint32_t
ft_get_mouse_state( int32_t* x, int32_t* y );

FT_API void
ft_window_get_vulkan_instance_extensions( const struct ft_window* window,
                                          uint32_t*               count,
                                          const char**            names );

FT_API void
ft_window_create_vulkan_surface( const struct ft_window*             window,
                                 VkInstance                          instance,
                                 const struct VkAllocationCallbacks* allocator,
                                 VkSurfaceKHR*                       surface );

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