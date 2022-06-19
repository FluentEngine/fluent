#include <SDL.h>
#include <SDL_vulkan.h>
#include "window.h"

struct ft_window
ft_create_window( const struct ft_window_info* info )
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

	struct ft_window result = { 0 };
	result.handle           = window;

	return result;
}

void
ft_destroy_window( struct ft_window* window )
{
	FT_ASSERT( window->handle && "Window is nullptr" );
	SDL_DestroyWindow( ( SDL_Window* ) window->handle );
	SDL_Quit();
}

void
ft_window_get_size( const struct ft_window* window,
                    uint32_t*               width,
                    uint32_t*               height )
{
	SDL_Window* handle = ( SDL_Window* ) ( window->handle );
	int32_t     w, h;

	SDL_GetWindowSize( handle, &w, &h );

	*width  = ( uint32_t ) w;
	*height = ( uint32_t ) h;
}

void
ft_window_get_framebuffer_size( const struct ft_window* window,
                                uint32_t*               width,
                                uint32_t*               height )
{
	SDL_Window* handle = ( SDL_Window* ) ( window->handle );
	int32_t     w, h;
#if defined( METAL_BACKEND )
	SDL_Metal_GetDrawableSize( handle, &w, &h );
#elif defined( VULKAN_BACKEND )
	SDL_Vulkan_GetDrawableSize( handle, &w, &h );
#endif

	*width  = ( uint32_t ) w;
	*height = ( uint32_t ) h;
}

uint32_t
ft_window_get_framebuffer_width( const struct ft_window* window )
{
	uint32_t w, h;
	ft_window_get_framebuffer_size( window, &w, &h );
	return w;
}

uint32_t
ft_window_get_framebuffer_height( const struct ft_window* window )
{
	uint32_t w, h;
	ft_window_get_framebuffer_size( window, &w, &h );
	return h;
}

uint32_t
ft_window_get_width( const struct ft_window* window )
{
	uint32_t w, h;
	ft_window_get_size( window, &w, &h );
	return w;
}

uint32_t
ft_window_get_height( const struct ft_window* window )
{
	uint32_t w, h;
	ft_window_get_size( window, &w, &h );
	return h;
}

float
ft_window_get_aspect( const struct ft_window* window )
{
	uint32_t w, h;
	ft_window_get_size( window, &w, &h );
	return ( float ) ( w ) / ( float ) ( h );
}

void
ft_window_show_cursor( bool show )
{
	SDL_SetRelativeMouseMode( show ? SDL_FALSE : SDL_TRUE );
}
