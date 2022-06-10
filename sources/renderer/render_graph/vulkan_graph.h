#pragma once

#include <hashmap_c/hashmap_c.h>
#include "../backend/vulkan/vulkan_backend.h"
#include "vulkan_pass_hasher.h"
#include "render_graph.h"

struct VulkanGraph;

struct VulkanGraphImage
{
	const char*       name;
	struct ImageInfo  info;
};

struct VulkanGraphPass
{
	const char*         name;
	struct VulkanGraph* graph;

	u32 color_output_count;
	u32 color_outputs[ MAX_ATTACHMENTS_COUNT ];
	
	struct RenderGraphPass interface;
};

struct VulkanPassInfo
{
	VkRenderPassCreateInfo       pass_info;
	struct VulkanFramebufferInfo framebuffer_info;
};

struct VulkanGraph
{
	const struct VulkanDevice* device;

	const struct VulkanImage* backbuffer_image;

	u32                      image_count;
	u32                      image_capacity;
	struct VulkanGraphImage* images;
	struct hashmap*          image_name_to_index;

	u32                      pass_count;
	u32                      pass_capacity;
	struct VulkanGraphPass** passes;

	u32                            physical_pass_count;
	struct VulkanPassInfo*         physical_passes;

	struct VulkanPassHasher pass_hasher;

	struct RenderGraph interface;
};

void
vk_rg_create( const struct Device*, struct RenderGraph** );
