#pragma once

#include <hashmap_c/hashmap_c.h>
#include "../backend/vulkan/vulkan_backend.h"

struct VulkanPassInfo
{
	const struct Device*  device;
	u32                   width;
	u32                   height;
	u32                   color_attachment_count;
	const struct Image*   color_attachments[ MAX_ATTACHMENTS_COUNT ];
	enum AttachmentLoadOp color_attachment_load_ops[ MAX_ATTACHMENTS_COUNT ];
	enum ResourceState    color_image_initial_states[ MAX_ATTACHMENTS_COUNT ];
	enum ResourceState    color_image_final_states[ MAX_ATTACHMENTS_COUNT ];
	const struct Image*   depth_stencil;
	enum AttachmentLoadOp depth_stencil_load_op;
	enum ResourceState    depth_stencil_initial_state;
	enum ResourceState    depth_stencil_final_state;
	struct ClearValue     clear_values[ MAX_ATTACHMENTS_COUNT + 1 ];
};

struct VulkanPassHasher
{
	const struct VulkanDevice* device;
	struct hashmap*      render_passes;
	struct hashmap*      framebuffers;
};

void
vk_pass_hasher_init( struct VulkanPassHasher*, const struct VulkanDevice* );

void
vk_pass_hasher_shutdown( struct VulkanPassHasher* );

void
vk_pass_hasher_framebuffers_clear( struct VulkanPassHasher* );

VkRenderPass
vk_pass_hasher_get_render_pass( struct VulkanPassHasher*,
                                const struct VulkanPassInfo* info );

VkFramebuffer
vk_pass_hasher_get_framebuffer( struct VulkanPassHasher*,
                                VkRenderPass,
                                const struct VulkanPassInfo* info );
