#pragma once

#include "core/base.hpp"

namespace fluent
{

enum WindowParams : u8
{
    ePositionX = 0,
    ePositionY = 1,
    eWidth = 2,
    eHeight = 3,
    eLast = 4
};

struct Window
{
    void* handle;
    u32 data[ static_cast<int>(WindowParams::eLast) ];
};

Window create_window(const char* title, u32 x, u32 y, u32 width, u32 height);
void destroy_window(Window& window);

u32 window_get_width(const Window* window);
u32 window_get_height(const Window* window);
f32 window_get_aspect(const Window* window);

} // namespace fluent