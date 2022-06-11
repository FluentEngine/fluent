#pragma once

#ifdef VULKAN_BACKEND

#include <hashmap_c/hashmap_c.h>
#include "vulkan_backend.h"
#include "vulkan_pass_hasher.h"
#include "../render_graph.h"

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

struct VulkanPhysicalPass
{
	u32                          graph_pass_index;
	struct VulkanRenderPassInfo  render_pass_info;
	struct VulkanFramebufferInfo framebuffer_info;
	VkClearValue                 clear_values[ MAX_ATTACHMENTS_COUNT + 1 ];
	VkRenderPassBeginInfo        begin_info;
	VkRenderPass                 render_pass;
	VkFramebuffer                framebuffer;
};

struct VulkanGraph
{
	const struct VulkanDevice* device;

	u32                      image_count;
	u32                      image_capacity;
	struct VulkanGraphImage* images;
	struct hashmap*          image_name_to_index;

	u32                      pass_count;
	u32                      pass_capacity;
	struct VulkanGraphPass** passes;

	u32                        physical_pass_count;
	struct VulkanPhysicalPass* physical_passes;
	u32                        swap_pass_index;

	struct VulkanPassHasher pass_hasher;

	u32 backbuffer_image_index;

	struct RenderGraph interface;
};

void
vk_rg_create( const struct Device*, struct RenderGraph** );

#endif
