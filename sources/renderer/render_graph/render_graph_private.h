#pragma once

#include "render_graph.h"

DECLARE_FUNCTION_POINTER( void, rg_destroy, struct RenderGraph* );

DECLARE_FUNCTION_POINTER( struct RenderGraphPass*,
                          rg_add_pass,
                          struct RenderGraph*,
                          const char* pass_name );

DECLARE_FUNCTION_POINTER( void,
                          rg_add_color_output,
                          struct RenderGraphPass*,
                          const struct ImageInfo*,
                          const char* name );

DECLARE_FUNCTION_POINTER( void, rg_build, struct RenderGraph* );

DECLARE_FUNCTION_POINTER( void,
                          rg_setup_attachments,
                          struct RenderGraph*,
                          const struct Image* );

DECLARE_FUNCTION_POINTER( void,
                          rg_execute,
                          struct RenderGraph*,
                          struct CommandBuffer* );
