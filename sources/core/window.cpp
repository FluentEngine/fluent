#include <SDL.h>
#include <SDL_vulkan.h>
#include "core/window.hpp"

namespace fluent
{

u32 Window::get_width() const
{
    return m_data[ 2 ];
}

u32 Window::get_height() const
{
    return m_data[ 3 ];
}

Window create_window(const WindowDescription& description)
{
    assert(description.width > 0 && description.height > 0 && "Width, height should be greater than zero");

    SDL_WindowFlags window_flags = ( SDL_WindowFlags ) (SDL_WINDOW_VULKAN);
    SDL_Window* window = SDL_CreateWindow(
        description.title, description.x, description.y, description.width, description.height, window_flags);

    assert(window && "Failed to create window");

    Window result{};
    result.m_handle = window;
    result.m_data[ 0 ] = description.x;
    result.m_data[ 1 ] = description.y;
    result.m_data[ 2 ] = description.width;
    result.m_data[ 3 ] = description.height;

    return result;
}

void destroy_window(Window& window)
{
    assert(window.m_handle && "Window is nullptr");
    SDL_DestroyWindow(( SDL_Window* ) window.m_handle);
}
} // namespace fluent