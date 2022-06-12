#include "renderer_backend.h"
#include "render_graph.h"

struct RenderPass
{
	struct RenderGraph* graph;
	const char*         name;
	u32                 physical_pass_index;

	void*                      user_data;
	create_cb                  create;
	execute_cb                 execute;
	destroy_cb                 destroy;
	get_clear_color_cb         clear_color;
	get_clear_depth_stencil_cb clear_depth_stencil;

	u32 color_attachment_count;
	u32 color_attachments[ MAX_ATTACHMENTS_COUNT ];
	b32 has_depth_stencil;
	u32 depth_stencil_attachment;
};

struct RenderGraph
{
	struct Device* device;

	u32                render_pass_count;
	u32                render_pass_capacity;
	struct RenderPass* render_passes;

	struct Image* swapchain_image;

	u32 execute_pass_count;
};

static void
default_create( void* user_data )
{
}

static void
default_execute( void* user_data )
{
}

static void
default_destroy( void* user_data )
{
}

static b32
default_clear_color( u32 idx, f32* colors[ 4 ] )
{
	return 0;
}

static b32
default_clear_depth_stencil( float* depth, u32* stencil )
{
	return 0;
}

void
rg_create( struct Device* device, struct RenderGraph** p )
{
	FT_ASSERT( device );

	struct RenderGraph* graph = calloc( 1, sizeof( struct RenderGraph ) );
	graph->device             = device;

	graph->render_pass_capacity = 1;
	graph->render_pass_count    = 0;
	graph->render_passes        = NULL;

	*p = graph;
}

void
rg_destroy( struct RenderGraph* graph )
{
	for ( u32 i = 0; i < graph->render_pass_count; ++i )
	{
		struct RenderPass* pass = &graph->render_passes[ i ];
		pass->destroy( pass->user_data );
	}
	free( graph->render_passes );
	free( graph );
}

void
rg_set_backbuffer_source( struct RenderGraph* graph, const char* name )
{
}

void
rg_build( struct RenderGraph* graph )
{
}

void
rg_setup_attachments( struct RenderGraph* graph, struct Image* image )
{
	graph->swapchain_image = image;
}

void
rg_execute( struct CommandBuffer* cmd, struct RenderGraph* graph )
{
	struct ImageBarrier barrier = {
		.image     = graph->swapchain_image,
		.old_state = FT_RESOURCE_STATE_UNDEFINED,
		.new_state = FT_RESOURCE_STATE_PRESENT,
	};

	cmd_barrier( cmd, 0, NULL, 0, NULL, 1, &barrier );
}

void
rg_add_pass( struct RenderGraph* graph,
             const char*         name,
             struct RenderPass** p )
{
	if ( graph->render_pass_count == ( graph->render_pass_capacity - 1 ) )
	{
		graph->render_pass_capacity *= 2;
		graph->render_passes = realloc( graph->render_passes,
		                                graph->render_pass_capacity *
		                                    sizeof( struct RenderPass ) );
	}

	struct RenderPass* pass =
	    &graph->render_passes[ graph->render_pass_count++ ];

	memset( pass, 0, sizeof( struct RenderPass ) );

	pass->name                = name;
	pass->graph               = graph;
	pass->create              = default_create;
	pass->execute             = default_execute;
	pass->destroy             = default_destroy;
	pass->clear_color         = default_clear_color;
	pass->clear_depth_stencil = default_clear_depth_stencil;

	pass->color_attachment_count = 0;

	*p = pass;
}

void
rg_add_color_output( struct RenderPass*      pass,
                     const char*             name,
                     const struct ImageInfo* info )
{
}

void
rg_set_user_data( struct RenderPass* pass, void* data )
{
	pass->user_data = data;
}

void
rg_set_pass_create_callback( struct RenderPass* pass, create_cb cb )
{
	FT_ASSERT( cb );
	pass->create = cb;
}

void
rg_set_pass_execute_callback( struct RenderPass* pass, execute_cb cb )
{
	FT_ASSERT( cb );
	pass->execute = cb;
}

void
rg_set_pass_destroy_callback( struct RenderPass* pass, destroy_cb cb )
{
	FT_ASSERT( cb );
	pass->destroy = cb;
}

void
rg_set_get_clear_color( struct RenderPass* pass, get_clear_color_cb cb )
{
	FT_ASSERT( cb );
	pass->clear_color = cb;
}

void
rg_set_get_clear_depth_stencil( struct RenderPass*         pass,
                                get_clear_depth_stencil_cb cb )
{
	FT_ASSERT( cb );
	pass->clear_depth_stencil = cb;
}
