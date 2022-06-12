#pragma once

#include "base/base.h"
#include "renderer/backend/renderer_enums.h"

#ifdef __cplusplus
extern "C"
{
#endif

	struct WindowInfo
	{
		const char*      title;
		u32              x;
		u32              y;
		u32              width;
		u32              height;
		b32              resizable;
		b32              centered;
		b32              fullscreen;
		b32              grab_mouse;
		enum RendererAPI renderer_api;
	};

	struct Window
	{
		void* handle;
	};

	struct Window
	create_window( const struct WindowInfo* info );

	void
	destroy_window( struct Window* window );

	void
	window_get_size( const struct Window* window, u32* width, u32* height );

	void
	window_get_framebuffer_size( const struct Window* window,
	                             u32*                 width,
	                             u32*                 height );

	u32
	window_get_framebuffer_width( const struct Window* window );

	u32
	window_get_framebuffer_height( const struct Window* window );

	u32
	window_get_width( const struct Window* window );
	u32
	window_get_height( const struct Window* window );
	f32
	window_get_aspect( const struct Window* window );
	void
	window_show_cursor( b32 show );

#ifdef __cplusplus
}
#endif
