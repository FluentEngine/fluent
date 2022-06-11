#pragma once

#include "render_graph.h"
#include "renderer_private.h"

struct NameToIndex
{
	const char* name;
	u32         idx;
};

struct hashmap*
create_name_to_index_map();

DECLARE_FUNCTION_POINTER( void, rg_destroy, struct RenderGraph* );

DECLARE_FUNCTION_POINTER( struct RenderGraphPass*,
                          rg_add_pass,
                          struct RenderGraph*,
                          const char* );

DECLARE_FUNCTION_POINTER( void,
                          rg_add_color_output,
                          struct RenderGraphPass*,
                          const struct ImageInfo*,
                          const char* );

DECLARE_FUNCTION_POINTER( void,
                          rg_set_backbuffer_source,
                          struct RenderGraph*,
                          const char* );

DECLARE_FUNCTION_POINTER( void, rg_build, struct RenderGraph* );

DECLARE_FUNCTION_POINTER( void,
                          rg_setup_attachments,
                          struct RenderGraph*,
                          const struct Image* );

DECLARE_FUNCTION_POINTER( void,
                          rg_execute,
                          struct RenderGraph*,
                          struct CommandBuffer* );