#pragma once

#include "core/base.hpp"

namespace fluent
{

enum WindowParams : u8
{
    ePositionX = 0,
    ePositionY = 1,
    eWidth     = 2,
    eHeight    = 3,
    eLast      = 4
};

struct WindowDesc
{
    const char* title      = nullptr;
    u32         x          = 0;
    u32         y          = 0;
    u32         width      = 0;
    u32         height     = 0;
    b32         grab_mouse = false;
};

struct Window
{
    void* handle;
    u32   data[ static_cast<int>(WindowParams::eLast) ];
};

Window create_window(const WindowDesc& desc);
void   destroy_window(Window& window);

u32  window_get_width(const Window* window);
u32  window_get_height(const Window* window);
f32  window_get_aspect(const Window* window);
void window_show_cursor(b32 show);

} // namespace fluent
