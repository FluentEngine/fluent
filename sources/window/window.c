#include "window/xlib/xlib_window.h"
#include "window/cocoa/cocoa_window.h"
#include "window/winapi/winapi_window.h"
#include "window_private.h"
#include "window.h"

ft_destroy_window_fun              ft_destroy_window_impl;
ft_window_set_resize_callback_fun  ft_window_set_resize_callback_impl;
ft_window_get_size_fun             ft_window_get_size_impl;
ft_window_get_framebuffer_size_fun ft_window_get_framebuffer_size_impl;
ft_window_show_cursor_fun          ft_window_show_cursor_impl;
ft_window_should_close_fun         ft_window_should_close_impl;
ft_window_get_vulkan_instance_extensions_fun
    ft_window_get_vulkan_instance_extensions_impl;
ft_window_create_vulkan_surface_fun ft_window_create_vulkan_surface_impl;
ft_poll_events_fun                  ft_poll_events_impl;
ft_get_keyboard_state_fun           ft_get_keyboard_state_impl;
ft_get_mouse_state_fun              ft_get_mouse_state_impl;

struct ft_window*
ft_create_window( const struct ft_window_info* info )
{
#if FT_PLATFORM_LINUX
	return xlib_create_window( info );
#endif
#if FT_PLATFORM_APPLE
	return cocoa_create_window( info );
#endif
#if FT_PLATFORM_WINDOWS
	return winapi_create_window( info );
#endif
	FT_WARN( "not supported platform" );
	return NULL;
}

void
ft_destroy_window( struct ft_window* window )
{
	FT_ASSERT( window );
	ft_destroy_window_impl( window );
}

void
ft_window_set_resize_callback( struct ft_window*         window,
                               ft_window_resize_callback cb )
{
	FT_ASSERT( window );
	FT_ASSERT( cb );

	ft_window_set_resize_callback_impl( window, cb );
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
ft_window_show_cursor( struct ft_window* window, bool show )
{
	ft_window_show_cursor_impl( window, show );
}

bool
ft_window_should_close( const struct ft_window* window )
{
	ft_window_should_close_impl( window );
}

void
ft_window_get_vulkan_instance_extensions( const struct ft_window* window,
                                          uint32_t*               count,
                                          const char**            names )
{
	ft_window_get_vulkan_instance_extensions_impl( window, count, names );
}

void
ft_window_create_vulkan_surface( const struct ft_window*             window,
                                 VkInstance                          instance,
                                 const struct VkAllocationCallbacks* allocator,
                                 VkSurfaceKHR*                       surface )
{
	ft_window_create_vulkan_surface_impl( window,
	                                      instance,
	                                      allocator,
	                                      surface );
}

void
ft_poll_events()
{
	ft_poll_events_impl();
}

const uint8_t*
ft_get_keyboard_state()
{
	return ft_get_keyboard_state_impl();
}

const uint8_t*
ft_get_mouse_state( int32_t* x, int32_t* y )
{
	return ft_get_mouse_state_impl( x, y );
}
