#pragma once

#include "base/base.h"

struct ft_window;

struct VkAllocationCallbacks;

struct ft_wsi_info
{
	const struct ft_window* window;

	void ( *get_window_size )( const struct ft_window* window,
	                           uint32_t*               width,
	                           uint32_t*               height );

	void ( *get_framebuffer_size )( const struct ft_window* window,
	                                uint32_t*               width,
	                                uint32_t*               height );

	void ( *get_vulkan_instance_extensions )( const struct ft_window* window,
	                                          uint32_t*               count,
	                                          const char**            names );

	void ( *create_vulkan_surface )( const struct ft_window* window,
	                                 VkInstance              instance,
	                                 const struct VkAllocationCallbacks*,
	                                 VkSurfaceKHR* surface );
};
