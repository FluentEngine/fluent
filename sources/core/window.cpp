#include <SDL.h>
#include <SDL_vulkan.h>
#include "core/window.hpp"

namespace fluent
{

Window
create_window( const WindowDesc& desc )
{
    int init_result = SDL_Init( SDL_INIT_VIDEO );
    FT_ASSERT( init_result == 0 && "SDL Init failed" );

    FT_ASSERT( desc.width > 0 && desc.height > 0 &&
               "Width, height should be greater than zero" );

    u32 window_flags = 0;
    u32 x            = desc.x;
    u32 y            = desc.y;

    window_flags |= SDL_WINDOW_ALLOW_HIGHDPI;

    if ( desc.resizable )
    {
        window_flags |= SDL_WINDOW_RESIZABLE;
    }

    if ( desc.fullscreen )
    {
        window_flags |= SDL_WINDOW_FULLSCREEN;
    }

    if ( desc.centered )
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

    SDL_Window* window = SDL_CreateWindow( desc.title,
                                           x,
                                           y,
                                           desc.width,
                                           desc.height,
                                           window_flags );

    FT_ASSERT( window && "Failed to create window" );

    Window result {};
    result.handle = window;

    return result;
}

void
destroy_window( Window& window )
{
    FT_ASSERT( window.handle && "Window is nullptr" );
    SDL_DestroyWindow( ( SDL_Window* ) window.handle );
    SDL_Quit();
}

void
window_get_size( const Window* window, u32* width, u32* height )
{
    SDL_Window* handle = static_cast<SDL_Window*>( window->handle );
    i32         w, h;
#if defined( METAL_BACKEND )
    SDL_Metal_GetDrawableSize( handle, &w, &h );
#elif defined( VULKAN_BACKEND )
    SDL_Vulkan_GetDrawableSize( handle, &w, &h );
#endif

    *width  = static_cast<u32>( w );
    *height = static_cast<u32>( h );
}

u32
window_get_width( const Window* window )
{
    u32 w, h;
    window_get_size( window, &w, &h );
    return w;
}

u32
window_get_height( const Window* window )
{
    u32 w, h;
    window_get_size( window, &w, &h );
    return h;
}

f32
window_get_aspect( const Window* window )
{
    u32 w, h;
    window_get_size( window, &w, &h );
    return static_cast<f32>( w ) / static_cast<f32>( h );
}

void
window_show_cursor( b32 show )
{
    SDL_SetRelativeMouseMode( show ? SDL_FALSE : SDL_TRUE );
}

} // namespace fluent
