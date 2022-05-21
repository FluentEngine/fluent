#pragma once

#include "base/base.h"

struct WsiInfo
{
	void*        window;
	u32          vulkan_instance_extension_count;
	const char** vulkan_instance_extensions;
	void ( *create_vulkan_surface )( void*  window,
	                                 void*  instance,
	                                 void** surface );
};
