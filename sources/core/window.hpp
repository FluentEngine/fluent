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
    void* m_handle;
    u32 m_data[ static_cast<int>(WindowParams::eLast) ];
};

Window create_window(const char* title, u32 x, u32 y, u32 width, u32 height);
void destroy_window(Window& window);

} // namespace fluent