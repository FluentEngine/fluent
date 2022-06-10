#pragma once

#include <hashmap_c/hashmap_c.h>
#include "../backend/vulkan/vulkan_backend.h"

struct VulkanPassHasher
{
	const struct VulkanDevice* device;
	struct hashmap*            render_passes;
	struct hashmap*            framebuffers;
};

struct VulkanSubpassInfo
{
	u32                   color_attachment_count;
	VkAttachmentReference color_attachment_references[ MAX_ATTACHMENTS_COUNT ];
	b32                   has_depth_stencil;
	VkAttachmentReference depth_attachment_reference;
};

struct VulkanRenderPassInfo
{
	u32                      attachment_count;
	VkAttachmentDescription  attachments[ MAX_ATTACHMENTS_COUNT + 1 ];
	u32                      subpass_count;
	struct VulkanSubpassInfo subpasses[ 1 ];
};

struct VulkanFramebufferInfo
{
	VkRenderPass render_pass;
	u32          attachment_count;
	VkImageView  attachments[ MAX_ATTACHMENTS_COUNT ];
	u32          width;
	u32          height;
	u32          layers;
};

void
vk_pass_hasher_init( struct VulkanPassHasher*, const struct VulkanDevice* );

void
vk_pass_hasher_shutdown( struct VulkanPassHasher* );

void
vk_pass_hasher_framebuffers_clear( struct VulkanPassHasher* );

VkRenderPass
vk_pass_hasher_get_render_pass( struct VulkanPassHasher*           hasher,
                                const struct VulkanRenderPassInfo* info );

VkFramebuffer
vk_pass_hasher_get_framebuffer( struct VulkanPassHasher*            hasher,
                                const struct VulkanFramebufferInfo* info );
