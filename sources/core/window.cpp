#include <SDL.h>
#include <SDL_vulkan.h>
#include "core/window.hpp"

namespace fluent
{

Window create_window( const WindowDesc& desc )
{
	int init_result = SDL_Init( SDL_INIT_VIDEO );
	FT_ASSERT( init_result == 0 && "SDL Init failed" );

    FT_ASSERT( desc.width > 0 && desc.height > 0 &&
               "Width, height should be greater than zero" );

    uint32_t window_flags = ( SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN );

    SDL_Window* window = SDL_CreateWindow( desc.title,
                                           desc.x,
                                           desc.y,
                                           desc.width,
                                           desc.height,
                                           window_flags );

    FT_ASSERT( window && "Failed to create window" );

    Window result {};
    result.handle                           = window;
    result.data[ WindowParams::ePositionX ] = desc.x;
    result.data[ WindowParams::ePositionY ] = desc.y;
    result.data[ WindowParams::eWidth ]     = desc.width;
    result.data[ WindowParams::eHeight ]    = desc.height;

    return result;
}

void destroy_window( Window& window )
{
    FT_ASSERT( window.handle && "Window is nullptr" );
    SDL_DestroyWindow( ( SDL_Window* ) window.handle );
	SDL_Quit();
}

u32 window_get_width( const Window* window )
{
    return window->data[ WindowParams::eWidth ];
}

u32 window_get_height( const Window* window )
{
    return window->data[ WindowParams::eHeight ];
}

f32 window_get_aspect( const Window* window )
{
    return ( f32 ) window->data[ WindowParams::eWidth ] /
           ( f32 ) window->data[ WindowParams::eHeight ];
}

void window_show_cursor( b32 show )
{
    SDL_SetRelativeMouseMode( show ? SDL_FALSE : SDL_TRUE );
}
} // namespace fluent
