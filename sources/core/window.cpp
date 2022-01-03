#include <SDL.h>
#include <SDL_vulkan.h>
#include "core/window.hpp"

namespace fluent
{

Window create_window(const char* title, u32 x, u32 y, u32 width, u32 height)
{
    FT_ASSERT(width > 0 && height > 0 && "Width, height should be greater than zero");

    SDL_WindowFlags window_flags = ( SDL_WindowFlags ) (SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
    SDL_Window* window = SDL_CreateWindow(title, x, y, width, height, window_flags);

    FT_ASSERT(window && "Failed to create window");

    Window result{};
    result.m_handle = window;
    result.m_data[ WindowParams::ePositionX ] = x;
    result.m_data[ WindowParams::ePositionY ] = y;
    result.m_data[ WindowParams::eWidth ] = width;
    result.m_data[ WindowParams::eHeight ] = height;

    return result;
}

void destroy_window(Window& window)
{
    FT_ASSERT(window.m_handle && "Window is nullptr");
    SDL_DestroyWindow(( SDL_Window* ) window.m_handle);
}

u32 window_get_width(const Window* window)
{
    return window->m_data[ WindowParams::eWidth ];
}

u32 window_get_height(const Window* window)
{
    return window->m_data[ WindowParams::eHeight ];
}

} // namespace fluent