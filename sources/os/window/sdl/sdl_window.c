#include "os/window/window_private.h"
#include "sdl_window.h"

#ifdef FT_WINDOW_SDL
#include <SDL.h>
#include <SDL_vulkan.h>

void
sdl_destroy_window( struct ft_window* window )
{
	SDL_DestroyWindow( ( SDL_Window* ) window );
	SDL_Quit();
}

static void
sdl_window_get_size( const struct ft_window* window,
                     uint32_t*               width,
                     uint32_t*               height )
{
	SDL_Window* handle = ( SDL_Window* ) window;
	int32_t     w, h;

	SDL_GetWindowSize( handle, &w, &h );

	*width  = ( uint32_t ) w;
	*height = ( uint32_t ) h;
}

static void
sdl_window_get_framebuffer_size( const struct ft_window* window,
                                 uint32_t*               width,
                                 uint32_t*               height )
{
	SDL_Window* handle = ( SDL_Window* ) window;
	int32_t     w, h;
#if defined( METAL_BACKEND )
	SDL_Metal_GetDrawableSize( handle, &w, &h );
#elif defined( VULKAN_BACKEND )
	SDL_Vulkan_GetDrawableSize( handle, &w, &h );
#endif

	*width  = ( uint32_t ) w;
	*height = ( uint32_t ) h;
}

static void
sdl_window_show_cursor( bool show )
{
	SDL_SetRelativeMouseMode( show ? SDL_FALSE : SDL_TRUE );
}

static void
sdl_window_get_vulkan_instance_extensions( const struct ft_window* window,
                                           uint32_t*               count,
                                           const char**            names )
{
	SDL_Vulkan_GetInstanceExtensions( ( SDL_Window* ) window, count, names );
}

static void
sdl_window_create_vulkan_surface( const struct ft_window*             window,
                                  VkInstance                          instance,
                                  const struct VkAllocationCallbacks* allocator,
                                  VkSurfaceKHR*                       surface )
{
	FT_UNUSED( allocator );
	SDL_Vulkan_CreateSurface( ( SDL_Window* ) window, instance, surface );
}

static int
sdl_window_poll_event( struct ft_event* event )
{
	SDL_Event e;
	int       r = SDL_PollEvent( &e );

	switch ( e.type )
	{
	case SDL_QUIT:
	{
		event->type = FT_EVENT_TYPE_QUIT;
		break;
	}
	case SDL_WINDOWEVENT:
	{
		event->type = FT_EVENT_TYPE_WINDOW;
		switch ( e.window.type )
		{
		case SDL_WINDOWEVENT_SIZE_CHANGED:
		{
			event->window.type = FT_WINDOW_EVENT_TYPE_RESIZE;
			break;
		}
		default:
		{
			break;
		}
		}
		break;
	}
	case SDL_MOUSEWHEEL:
	{
		event->type            = FT_EVENT_TYPE_MOUSEWHEEL;
		event->wheel.direction = e.wheel.direction == SDL_MOUSEWHEEL_NORMAL
		                             ? FT_MOUSE_WHEEL_DIRECTION_NORMAL
		                             : FT_MOUSE_WHEEL_DIRECTION_FLIPPED;
		event->wheel.y         = e.wheel.y;
		break;
	}
	default:
	{
		break;
	}
	}

	return r;
}

static void
sdl_get_mouse_offset( int32_t* x, int32_t* y )
{
	SDL_GetRelativeMouseState( x, y );
}

static const uint8_t*
sdl_get_keyboard_state( uint32_t* key_count )
{
	int      count;
	const uint8_t* r = SDL_GetKeyboardState( &count );
	*key_count = count;
	return r;
}

static uint32_t
sdl_get_mouse_state( int32_t* x, int32_t* y )
{
	return SDL_GetGlobalMouseState( x, y );
}

struct ft_window*
sdl_create_window( const struct ft_window_info* info )
{
	int init_result = SDL_Init( SDL_INIT_VIDEO );
	FT_ASSERT( init_result == 0 && "SDL Init failed" );

	FT_ASSERT( info->width > 0 && info->height > 0 &&
	           "Width, height should be greater than zero" );

	uint32_t window_flags = 0;
	uint32_t x            = info->x;
	uint32_t y            = info->y;

	//	window_flags |= SDL_WINDOW_ALLOW_HIGHDPI;

	if ( info->resizable )
	{
		window_flags |= SDL_WINDOW_RESIZABLE;
	}

	if ( info->fullscreen )
	{
		window_flags |= SDL_WINDOW_FULLSCREEN;
	}

	if ( info->centered )
	{
		x = SDL_WINDOWPOS_CENTERED;
		y = SDL_WINDOWPOS_CENTERED;
	}

	switch ( info->renderer_api )
	{
	case FT_RENDERER_API_VULKAN:
	{
#ifdef VULKAN_BACKEND
		window_flags |= SDL_WINDOW_VULKAN;
#endif
		break;
	}
	case FT_RENDERER_API_METAL:
	{
#ifdef METAL_BACKEND
		window_flags |= SDL_WINDOW_METAL;
#endif
		break;
	}
	default:
	{
		break;
	}
	}

	SDL_Window* window = SDL_CreateWindow( info->title,
	                                       x,
	                                       y,
	                                       info->width,
	                                       info->height,
	                                       window_flags );

	FT_ASSERT( window && "Failed to create window" );

	ft_window_get_size_impl             = sdl_window_get_size;
	ft_window_get_framebuffer_size_impl = sdl_window_get_framebuffer_size;
	ft_destroy_window_impl              = sdl_destroy_window;
	ft_window_show_cursor_impl          = sdl_window_show_cursor;
	ft_window_poll_event_impl           = sdl_window_poll_event;
	ft_get_mouse_offset_impl            = sdl_get_mouse_offset;
	ft_get_keyboard_state_impl          = sdl_get_keyboard_state;
	ft_get_mouse_state_impl             = sdl_get_mouse_state;
	ft_window_get_vulkan_instance_extensions_impl =
	    sdl_window_get_vulkan_instance_extensions;
	ft_window_create_vulkan_surface_impl = sdl_window_create_vulkan_surface;

	return ( struct ft_window* ) window;
}
#endif
