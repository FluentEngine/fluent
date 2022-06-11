#pragma once

#include <hashmap_c/hashmap_c.h>
#include "metal_backend.h"
#include "../render_graph.h"

struct MetalGraph
{
	const struct MetalDevice* device;

	struct RenderGraph interface;
};

void
mtl_rg_create( const struct Device*, struct RenderGraph** );
