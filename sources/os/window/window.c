#include "os/window/sdl/sdl_window.h"
#include "os/window/cocoa/cocoa_window.h"
#include "window_private.h"
#include "window.h"

ft_destroy_window_fun              ft_destroy_window_impl;
ft_window_get_size_fun             ft_window_get_size_impl;
ft_window_get_framebuffer_size_fun ft_window_get_framebuffer_size_impl;
ft_window_show_cursor_fun          ft_window_show_cursor_impl;
ft_window_poll_event_fun           ft_window_poll_event_impl;
ft_get_mouse_offset_fun            ft_get_mouse_offset_impl;
ft_get_keyboard_state_fun          ft_get_keyboard_state_impl;
ft_get_mouse_state_fun             ft_get_mouse_state_impl;

ft_window_get_vulkan_instance_extensions_fun
    ft_window_get_vulkan_instance_extensions_impl;
ft_window_create_vulkan_surface_fun ft_window_create_vulkan_surface_impl;

struct ft_window*
ft_create_window( const struct ft_window_info* info )
{
#ifdef __APPLE__
	return cocoa_create_window( info );
#endif
#ifdef FT_WINDOW_SDL
	return sdl_create_window( info );
#endif
	return NULL;
}

void
ft_destroy_window( struct ft_window* window )
{
	FT_ASSERT( window && "Window is nullptr" );
	ft_destroy_window_impl( window );
}

void
ft_window_get_size( const struct ft_window* window,
                    uint32_t*               width,
                    uint32_t*               height )
{
	ft_window_get_size_impl( window, width, height );
}

void
ft_window_get_framebuffer_size( const struct ft_window* window,
                                uint32_t*               width,
                                uint32_t*               height )
{
	ft_window_get_framebuffer_size_impl( window, width, height );
}

void
ft_window_show_cursor( bool show )
{
	ft_window_show_cursor_impl( show );
}

int
ft_window_poll_event( struct ft_event* event )
{
	return ft_window_poll_event_impl( event );
}

void
ft_get_mouse_offset( int32_t* x, int32_t* y )
{
	ft_get_mouse_offset_impl( x, y );
}

const uint8_t*
ft_get_keyboard_state( uint32_t* key_count )
{
	return ft_get_keyboard_state_impl( key_count );
}

uint32_t
ft_get_mouse_state( int32_t* x, int32_t* y )
{
	return ft_get_mouse_state_impl( x, y );
}

void
ft_window_get_vulkan_instance_extensions( const struct ft_window* window,
                                          uint32_t*               count,
                                          const char**            names )
{
	ft_window_get_vulkan_instance_extensions_impl( window, count, names );
}

void
ft_window_create_vulkan_surface( const struct ft_window* window,
                                 VkInstance              instance,
                                 VkSurfaceKHR*           surface )
{
	ft_window_create_vulkan_surface_impl( window, instance, surface );
}