#include "log/log.h"
#include "../render_graph_private.h"
#include "metal_graph.h"

static struct RenderGraphPass*
mtl_rg_add_pass( struct RenderGraph* igraph, const char* name )
{
	return NULL;
}

static void
mtl_rg_add_color_output( struct RenderGraphPass* ipass,
                         const struct ImageInfo* info,
                         const char*             name )
{
}

static void
mtl_rg_set_backbuffer_source( struct RenderGraph* igraph, const char* name )
{
}

static void
mtl_rg_build( struct RenderGraph* igraph )
{
}

static void
mtl_rg_setup_attachments( struct RenderGraph* igraph,
                          const struct Image* iimage )
{
}

static void
mtl_rg_execute( struct RenderGraph* igraph, struct CommandBuffer* icmd )
{
}

static void
mtl_rg_destroy( struct RenderGraph* igraph )
{
	FT_FROM_HANDLE( graph, igraph, MetalGraph );
	free( graph );
}

void
mtl_rg_create( const struct Device* idevice, struct RenderGraph** p )
{
	FT_FROM_HANDLE( device, idevice, MetalDevice );

	FT_INIT_INTERNAL( graph, *p, MetalGraph );

	rg_destroy_impl               = mtl_rg_destroy;
	rg_add_pass_impl              = mtl_rg_add_pass;
	rg_add_color_output_impl      = mtl_rg_add_color_output;
	rg_set_backbuffer_source_impl = mtl_rg_set_backbuffer_source;
	rg_build_impl                 = mtl_rg_build;
	rg_setup_attachments_impl     = mtl_rg_setup_attachments;
	rg_execute_impl               = mtl_rg_execute;
}
