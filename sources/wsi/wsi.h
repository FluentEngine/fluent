#ifndef WSI_H
#define WSI_H

#include "base/base.h"

typedef struct WsiInfo
{
	void*        window;
	u32          vulkan_instance_extension_count;
	const char** vulkan_instance_extensions;
	void ( *create_vulkan_surface )( void*  window,
	                                 void*  instance,
	                                 void** surface );
	void ( *imgui_vulkan_init )( void* window );
	void ( *imgui_new_frame )();
	void ( *imgui_shutdown )();
} WsiInfo;

#endif
