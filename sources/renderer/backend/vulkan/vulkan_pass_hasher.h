#pragma once

#ifdef VULKAN_BACKEND

#include <hashmap_c/hashmap_c.h>
#include "vulkan_backend.h"

struct vulkan_pass_hasher;

void
vk_pass_hasher_init( const struct vk_device* );

void
vk_pass_hasher_shutdown();

void
vk_pass_hasher_framebuffers_clear();

VkRenderPass
vk_pass_hasher_get_render_pass( const struct ft_render_pass_begin_info* info );

VkFramebuffer
vk_pass_hasher_get_framebuffer( VkRenderPass render_pass,
                                const struct ft_render_pass_begin_info* info );

void
vk_create_render_pass( const struct vk_device*                 device,
                       const struct ft_render_pass_begin_info* info,
                       VkRenderPass*                           p );
#endif
