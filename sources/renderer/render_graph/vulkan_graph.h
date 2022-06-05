#pragma once

#include <hashmap_c/hashmap_c.h>
#include "../backend/vulkan/vulkan_backend.h"
#include "render_graph.h"

struct VulkanGraph;

struct VulkanGraphPass
{
	struct RenderGraphPass interface;
};

struct VulkanGraph
{
	const struct VulkanDevice* device;
	const struct VulkanImage*  backbuffer_image;
	struct RenderGraph         interface;
};

void
vk_rg_create( const struct Device*, struct RenderGraph** );
