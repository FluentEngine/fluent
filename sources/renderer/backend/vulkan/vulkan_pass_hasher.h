#pragma once

#ifdef VULKAN_BACKEND

#include <hashmap_c/hashmap_c.h>
#include "vulkan_backend.h"

struct VulkanPassHasher;

void
vk_pass_hasher_init( const struct VulkanDevice* );

void
vk_pass_hasher_shutdown();

void
vk_pass_hasher_framebuffers_clear();

VkRenderPass
vk_pass_hasher_get_render_pass( const struct RenderPassBeginInfo* info );

VkFramebuffer
vk_pass_hasher_get_framebuffer(  VkRenderPass render_pass, const struct RenderPassBeginInfo* info );

void
vk_create_render_pass( const struct VulkanDevice*        device,
                       const struct RenderPassBeginInfo* info,
                       VkRenderPass*                     p );
#endif
