#pragma once

#include <hashmap_c/hashmap_c.h>
#include "../backend/vulkan/vulkan_backend.h"

struct VulkanPassHasher
{
	const struct VulkanDevice* device;
	struct hashmap*            render_passes;
	struct hashmap*            framebuffers;
};

void
vk_pass_hasher_init( struct VulkanPassHasher*, const struct VulkanDevice* );

void
vk_pass_hasher_shutdown( struct VulkanPassHasher* );

void
vk_pass_hasher_framebuffers_clear( struct VulkanPassHasher* );

VkRenderPass
vk_pass_hasher_get_render_pass( struct VulkanPassHasher*      hasher,
                                const VkRenderPassCreateInfo* info );

VkFramebuffer
vk_pass_hasher_get_framebuffer( struct VulkanPassHasher*       hasher,
                                const VkFramebufferCreateInfo* info );
