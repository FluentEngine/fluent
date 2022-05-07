#pragma once

#include "core/base.hpp"

namespace fluent
{

struct WindowInfo
{
	const char* title      = nullptr;
	u32         x          = 0;
	u32         y          = 0;
	u32         width      = 0;
	u32         height     = 0;
	b32         resizable  = false;
	b32         centered   = false;
	b32         fullscreen = false;
	b32         grab_mouse = false;
};

struct Window
{
	void* handle;
};

Window
create_window( const WindowInfo& info );
void
destroy_window( Window& window );

void
window_get_size( const Window* window, u32* width, u32* height );
u32
window_get_width( const Window* window );
u32
window_get_height( const Window* window );
f32
window_get_aspect( const Window* window );
void
window_show_cursor( b32 show );

} // namespace fluent
