#pragma once

#include "base/base.h"

struct ft_wsi_info
{
	void*        window;
	uint32_t     vulkan_instance_extension_count;
	const char** vulkan_instance_extensions;

	void ( *get_window_size )( void*     window,
	                           uint32_t* width,
	                           uint32_t* height );

	void ( *get_framebuffer_size )( void*     window,
	                                uint32_t* width,
	                                uint32_t* height );

	void ( *create_vulkan_surface )( void*  window,
	                                 void*  instance,
	                                 void** surface );
};
