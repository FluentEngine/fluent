#pragma once

#include "base/base.h"

struct WsiInfo
{
	void*        window;
	u32          vulkan_instance_extension_count;
	const char** vulkan_instance_extensions;
	void ( *get_window_size )( void* window, u32* width, u32* height );
	void ( *get_framebuffer_size )( void* window, u32* width, u32* height );
	void ( *create_vulkan_surface )( void*  window,
	                                 void*  instance,
	                                 void** surface );
};
