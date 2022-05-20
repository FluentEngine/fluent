#pragma once

#include "base/base.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct WindowInfo
	{
		const char* title;
		u32         x;
		u32         y;
		u32         width;
		u32         height;
		b32         resizable;
		b32         centered;
		b32         fullscreen;
		b32         grab_mouse;
	} WindowInfo;

	typedef struct Window
	{
		void* handle;
	} Window;

	Window
	create_window( const WindowInfo* info );

	void
	destroy_window( Window* window );

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

#ifdef __cplusplus
}
#endif
