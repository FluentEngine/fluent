#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include "window.h"

struct Window
create_window( const struct WindowInfo* info )
{
	int init_result = SDL_Init( SDL_INIT_VIDEO );
	FT_ASSERT( init_result == 0 && "SDL Init failed" );

	FT_ASSERT( info->width > 0 && info->height > 0 &&
	           "Width, height should be greater than zero" );

	u32 window_flags = 0;
	u32 x            = info->x;
	u32 y            = info->y;

	window_flags |= SDL_WINDOW_ALLOW_HIGHDPI;

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

#ifdef VULKAN_BACKEND
	window_flags |= SDL_WINDOW_VULKAN;
#endif

#ifdef METAL_BACKEND
	window_flags |= SDL_WINDOW_METAL;
#endif

	SDL_Window* window = SDL_CreateWindow( info->title,
	                                       x,
	                                       y,
	                                       info->width,
	                                       info->height,
	                                       window_flags );

	FT_ASSERT( window && "Failed to create window" );

	SDL_SetWindowOpacity( window, 0.1f );
	struct Window result = {};
	result.handle        = window;

	return result;
}

void
destroy_window( struct Window* window )
{
	FT_ASSERT( window->handle && "Window is nullptr" );
	SDL_DestroyWindow( ( SDL_Window* ) window->handle );
	SDL_Quit();
}

void
window_get_size( const struct Window* window, u32* width, u32* height )
{
	SDL_Window* handle = ( SDL_Window* ) ( window->handle );
	i32         w, h;
#if defined( METAL_BACKEND )
	SDL_Metal_GetDrawableSize( handle, &w, &h );
#elif defined( VULKAN_BACKEND )
	SDL_Vulkan_GetDrawableSize( handle, &w, &h );
#endif

	*width  = ( u32 ) w;
	*height = ( u32 ) h;
}

u32
window_get_width( const struct Window* window )
{
	u32 w, h;
	window_get_size( window, &w, &h );
	return w;
}

u32
window_get_height( const struct Window* window )
{
	u32 w, h;
	window_get_size( window, &w, &h );
	return h;
}

f32
window_get_aspect( const struct Window* window )
{
	u32 w, h;
	window_get_size( window, &w, &h );
	return ( f32 ) ( w ) / ( f32 ) ( h );
}

void
window_show_cursor( b32 show )
{
	SDL_SetRelativeMouseMode( show ? SDL_FALSE : SDL_TRUE );
}
