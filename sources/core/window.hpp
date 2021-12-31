#pragma once

#include "core/base.hpp"

namespace fluent
{

struct WindowDescription
{
    const char* title = nullptr;
    u32 x;
    u32 y;
    u32 width;
    u32 height;
};

struct Window
{
    void* m_handle;
    u32 m_data[ 4 ];

    u32 get_width() const;
    u32 get_height() const;
};

Window create_window(const WindowDescription& description);
void destroy_window(Window& window);
} // namespace fluent