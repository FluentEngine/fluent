#include "os/window/window_private.h"
#include "cocoa_window.h"

#ifdef __APPLE__

#import <Cocoa/Cocoa.h>

void
cocoa_destroy_window( struct ft_window* window )
{
}

static void
cocoa_window_get_size( const struct ft_window* window,
                       uint32_t*               width,
                       uint32_t*               height )
{
}

static void
cocoa_window_get_framebuffer_size( const struct ft_window* window,
                                   uint32_t*               width,
                                   uint32_t*               height )
{
}

static void
cocoa_window_show_cursor( bool show )
{
}

static void
cocoa_window_get_vulkan_instance_extensions( const struct ft_window* window,
                                             uint32_t*               count,
                                             const char**            names )
{
}

static void
cocoa_window_create_vulkan_surface( const struct ft_window* window,
                                    VkInstance              instance,
                                    VkSurfaceKHR*           surface )
{
}

static int
cocoa_window_poll_event( struct ft_event* event )
{
	return 0;
}

static void
cocoa_get_mouse_offset( int32_t* x, int32_t* y )
{
}

static const uint8_t*
cocoa_get_keyboard_state( uint32_t* key_count )
{
	return NULL;
}

static uint32_t
cocoa_get_mouse_state( int32_t* x, int32_t* y )
{
	return 0;
}

struct ft_window*
cocoa_create_window( const struct ft_window_info* info )
{
	ft_window_get_size_impl             = cocoa_window_get_size;
	ft_window_get_framebuffer_size_impl = cocoa_window_get_framebuffer_size;
	ft_destroy_window_impl              = cocoa_destroy_window;
	ft_window_show_cursor_impl          = cocoa_window_show_cursor;
	ft_window_poll_event_impl           = cocoa_window_poll_event;
	ft_get_mouse_offset_impl            = cocoa_get_mouse_offset;
	ft_get_keyboard_state_impl          = cocoa_get_keyboard_state;
	ft_get_mouse_state_impl             = cocoa_get_mouse_state;
	ft_window_get_vulkan_instance_extensions_impl =
	    cocoa_window_get_vulkan_instance_extensions;
	ft_window_create_vulkan_surface_impl = cocoa_window_create_vulkan_surface;

	return NULL;
}

#endif
