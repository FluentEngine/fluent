#include "cocoa_window.h"

#if FT_PLATFORM_APPLE

#import <Cocoa/Cocoa.h>

#define VK_USE_PLATFORM_MACOS_MVK
#include <volk/volk.h>
#include "log/log.h"
#include "os/window/window_private.h"

struct ft_window
{
	NSWindow* window;
	uint32_t  width;
	uint32_t  height;
};

static const char* cocoa_vulkan_extension_names[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_MVK_MACOS_SURFACE_EXTENSION_NAME };

void
cocoa_destroy_window( struct ft_window* window )
{
	free( window );
}

static void
cocoa_window_get_size( const struct ft_window* window,
                       uint32_t*               width,
                       uint32_t*               height )
{
	*width  = window->width;
	*height = window->height;
}

static void
cocoa_window_get_framebuffer_size( const struct ft_window* window,
                                   uint32_t*               width,
                                   uint32_t*               height )
{
	*width  = window->width;
	*height = window->height;
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
	if ( names == NULL )
	{
		*count = FT_ARRAY_SIZE( cocoa_vulkan_extension_names );
	}
	else
	{
		for ( uint32_t i = 0; i < *count; ++i )
			names[ i ] = cocoa_vulkan_extension_names[ i ];
	}
}

static void
cocoa_window_create_vulkan_surface(
    const struct ft_window*             window,
    VkInstance                          instance,
    const struct VkAllocationCallbacks* allocator,
    VkSurfaceKHR*                       surface )
{
}

static int
cocoa_window_poll_event( struct ft_event* event )
{
	NSEvent* e;
	e = [[NSApplication sharedApplication] nextEventMatchingMask:NSEventMaskAny
	                       untilDate:[NSDate distantPast]
	                          inMode:NSDefaultRunLoopMode
	                         dequeue:YES]];
	[NSApp sendEvent:e];
	return e != nil;
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

	[NSApplication sharedApplication];
	NSRect    rect = NSMakeRect( info->x, info->y, info->width, info->height );
	NSWindow* window =
	    [[NSWindow alloc] initWithContentRect:rect
	                                styleMask:NSWindowStyleMaskBorderless
	                                  backing:NSBackingStoreBuffered
	                                    defer:NO];
	[window makeKeyAndOrderFront:NSApp];

	struct ft_window* r = malloc( sizeof( struct ft_window ) );
	r->window           = window;
	r->width            = info->width;
	r->height           = info->height;

	return r;
}

#endif
