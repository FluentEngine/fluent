#pragma once

#include "base/base.h"
#include "../backend/renderer_backend.h"

struct RenderGraphPass
{
	ft_handle handle;
};

struct RenderGraph
{
	ft_handle handle;
};

void
rg_create( const struct Device*, struct RenderGraph** );

void
rg_destroy( struct RenderGraph* );

struct RenderGraphPass*
rg_add_pass( struct RenderGraph* graph, const char* pass_name );

void
rg_add_color_output( struct RenderGraphPass* pass,
                     const struct ImageInfo* image_info,
                     const char*             name );

void
rg_build( struct RenderGraph* graph );

void
rg_setup_attachments( struct RenderGraph* graph, const struct Image* image );

void
rg_execute( struct RenderGraph* graph, struct CommandBuffer* cmd );
