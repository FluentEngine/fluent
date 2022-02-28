#include <SDL.h>
#include <SDL_vulkan.h>
#include "core/window.hpp"

namespace fluent
{

Window create_window(const WindowDesc& desc)
{
    FT_ASSERT(
        desc.width > 0 && desc.height > 0 &&
        "Width, height should be greater than zero");

    SDL_WindowFlags window_flags =
        ( SDL_WindowFlags ) (SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

    if (desc.grab_mouse)
    {
        window_flags =
            ( SDL_WindowFlags ) (window_flags | SDL_WINDOW_MOUSE_GRABBED);
    }

    SDL_Window* window = SDL_CreateWindow(
        desc.title, desc.x, desc.y, desc.width, desc.height, window_flags);

    FT_ASSERT(window && "Failed to create window");

    Window result{};
    result.handle                           = window;
    result.data[ WindowParams::ePositionX ] = desc.x;
    result.data[ WindowParams::ePositionY ] = desc.y;
    result.data[ WindowParams::eWidth ]     = desc.width;
    result.data[ WindowParams::eHeight ]    = desc.height;

    return result;
}

void destroy_window(Window& window)
{
    FT_ASSERT(window.handle && "Window is nullptr");
    SDL_DestroyWindow(( SDL_Window* ) window.handle);
}

u32 window_get_width(const Window* window)
{
    return window->data[ WindowParams::eWidth ];
}

u32 window_get_height(const Window* window)
{
    return window->data[ WindowParams::eHeight ];
}

f32 window_get_aspect(const Window* window)
{
    return ( f32 ) window->data[ WindowParams::eWidth ] /
           ( f32 ) window->data[ WindowParams::eHeight ];
}

} // namespace fluent
