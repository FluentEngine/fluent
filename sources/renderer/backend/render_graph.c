#include <hashmap_c/hashmap_c.h>
#include "renderer_backend.h"
#include "render_graph.h"

struct NameToIndex
{
	const char* name;
	u32         index;
};

static b32
compare_name_to_index( const void* a, const void* b, void* udata )
{
	FT_UNUSED( udata );

	const struct NameToIndex* na = a;
	const struct NameToIndex* nb = b;

	return strcmp( na->name, nb->name );
}

static u64
hash_name_to_index( const void* item, u64 seed0, u64 seed1 )
{
	FT_UNUSED( item );
	FT_UNUSED( seed0 );
	FT_UNUSED( seed1 );
	return 0;
}

static inline struct hashmap*
create_name_to_index_map()
{
	return hashmap_new( sizeof( struct NameToIndex ),
	                    0,
	                    0,
	                    0,
	                    hash_name_to_index,
	                    compare_name_to_index,
	                    NULL,
	                    NULL );
}

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

struct PassBarriers
{
	u32                  image_barrier_count;
	struct ImageBarrier* image_barriers;
};

struct RenderGraph
{
	struct Device* device;

	u32                render_pass_count;
	u32                render_pass_capacity;
	struct RenderPass* render_passes;

	struct hashmap*   image_name_to_index;
	u32               image_count;
	u32               image_capacity;
	struct ImageInfo* images;

	struct Image* swapchain_image;
	u32           swapchain_image_index;
	u32           swapchain_pass_index;
	u32           swapchain_image_width;
	u32           swapchain_image_height;

	u32                         physical_pass_count;
	struct RenderPassBeginInfo* physical_passes;
	u32                         physical_image_count;
	struct Image**              physical_images;
	struct PassBarriers*        pass_barriers;
};

static void
default_create( const struct Device* device, void* user_data )
{
	FT_UNUSED( device );
	FT_UNUSED( user_data );
}

static void
default_execute( const struct Device*  device,
                 struct CommandBuffer* cmd,
                 void*                 user_data )
{
	FT_UNUSED( device );
	FT_UNUSED( cmd );
	FT_UNUSED( user_data );
}

static void
default_destroy( const struct Device* device, void* user_data )
{
	FT_UNUSED( device );
	FT_UNUSED( user_data );
}

static b32
default_clear_color( u32 idx, ColorClearValue* color )
{
	FT_UNUSED( idx );
	FT_UNUSED( color );
	return 0;
}

static b32
default_clear_depth_stencil( struct DepthStencilClearValue* depth_stencil )
{
	FT_UNUSED( depth_stencil );
	return 0;
}

static u32
rg_get_image( struct RenderGraph* graph,
              const char*         name,
              struct ImageInfo**  info )
{
	struct NameToIndex* it = hashmap_get( graph->image_name_to_index,
	                                      &( struct NameToIndex ) {
	                                          .name = name,
	                                      } );

	if ( it != NULL )
	{
		*info = &graph->images[ it->index ];
		return it->index;
	}
	else
	{
		hashmap_set( graph->image_name_to_index,
		             &( struct NameToIndex ) {
		                 .name  = name,
		                 .index = graph->image_count,
		             } );

		if ( graph->image_count == graph->image_capacity - 1 )
		{
			graph->image_capacity *= 2;
			graph->images =
			    realloc( graph->images,
			             graph->image_capacity * sizeof( struct ImageInfo ) );
		}

		memset( &graph->images[ graph->image_count ],
		        0,
		        sizeof( struct ImageInfo ) );

		*info = &graph->images[ graph->image_count ];

		return graph->image_count++;
	}
}

void
rg_create( struct Device* device, struct RenderGraph** p )
{
	FT_ASSERT( device );

	struct RenderGraph* graph   = calloc( 1, sizeof( struct RenderGraph ) );
	graph->device               = device;
	graph->render_pass_capacity = 1;
	graph->image_capacity       = 1;
	graph->image_name_to_index  = create_name_to_index_map();

	*p = graph;
}

void
rg_destroy( struct RenderGraph* graph )
{
	if ( graph->pass_barriers )
	{
		for ( u32 p = 0; p < graph->render_pass_count; ++p )
		{
			if ( graph->pass_barriers[ p ].image_barriers )
			{
				free( graph->pass_barriers[ p ].image_barriers );
			}
		}
		free( graph->pass_barriers );
	}

	if ( graph->physical_images )
	{
		for ( u32 i = 0; i < graph->physical_image_count; ++i )
		{
			if ( graph->swapchain_image_index != i )
			{
				destroy_image( graph->device, graph->physical_images[ i ] );
			}
		}
		free( graph->physical_images );
	}

	if ( graph->physical_passes )
	{
		free( graph->physical_passes );
	}

	hashmap_free( graph->image_name_to_index );

	if ( graph->images )
	{
		free( graph->images );
	}

	for ( u32 i = 0; i < graph->render_pass_count; ++i )
	{
		struct RenderPass* pass = &graph->render_passes[ i ];
		pass->destroy( graph->device, pass->user_data );
	}

	if ( graph->render_passes )
	{
		free( graph->render_passes );
	}

	free( graph );
}

void
rg_set_backbuffer_source( struct RenderGraph* graph, const char* name )
{
	struct NameToIndex* it       = hashmap_get( graph->image_name_to_index,
                                          &( struct NameToIndex ) {
	                                                .name = name,
                                          } );
	graph->swapchain_image_index = it->index;
}

static inline void
rg_create_physical_images( struct RenderGraph* graph )
{
	if ( graph->physical_images )
	{
		free( graph->physical_images );
	}

	graph->physical_image_count = graph->image_count;
	graph->physical_images =
	    calloc( graph->physical_image_count, sizeof( struct Image* ) );

	for ( u32 p = 0; p < graph->render_pass_count; ++p )
	{
		struct RenderPass* pass = &graph->render_passes[ p ];
		for ( u32 i = 0; i < pass->color_attachment_count; ++i )
		{
			if ( pass->color_attachments[ i ] == graph->swapchain_image_index )
			{
				graph->swapchain_pass_index = p;
			}
			else
			{
				create_image(
				    graph->device,
				    &graph->images[ pass->color_attachments[ i ] ],
				    &graph->physical_images[ pass->color_attachments[ i ] ] );
			}
		}

		if ( pass->has_depth_stencil )
		{
			create_image(
			    graph->device,
			    &graph->images[ pass->depth_stencil_attachment ],
			    &graph->physical_images[ pass->depth_stencil_attachment ] );
		}
	}
}

static void
rg_build_pass_barriers( struct RenderGraph* graph )
{
	if ( graph->render_pass_count == 0 )
	{
		return;
	}

	if ( graph->pass_barriers )
	{
		for ( u32 p = 0; p < graph->render_pass_count; ++p )
		{
			if ( graph->pass_barriers[ p ].image_barriers )
			{
				free( graph->pass_barriers[ p ].image_barriers );
			}
		}
		free( graph->pass_barriers );
	}

	graph->pass_barriers =
	    calloc( graph->render_pass_count, sizeof( struct PassBarriers ) );

	for ( u32 p = 0; p < graph->render_pass_count; ++p )
	{
		struct RenderPass*   pass     = &graph->render_passes[ p ];
		struct PassBarriers* barriers = &graph->pass_barriers[ p ];
		barriers->image_barrier_count =
		    pass->color_attachment_count + pass->has_depth_stencil;

		if ( barriers->image_barrier_count == 0 )
		{
			continue;
		}

		barriers->image_barriers = calloc( barriers->image_barrier_count,
		                                   sizeof( struct ImageBarrier ) );
		for ( u32 a = 0; a < pass->color_attachment_count; ++a )
		{
			barriers->image_barriers[ a ].image =
			    graph->physical_images[ pass->color_attachments[ a ] ];
			barriers->image_barriers[ a ].old_state =
			    FT_RESOURCE_STATE_UNDEFINED;
			barriers->image_barriers[ a ].new_state =
			    FT_RESOURCE_STATE_COLOR_ATTACHMENT;
		}

		if ( pass->has_depth_stencil )
		{
			u32 a = pass->color_attachment_count;
			barriers->image_barriers[ a ].image =
			    graph->physical_images[ pass->depth_stencil_attachment ];
			barriers->image_barriers[ a ].old_state =
			    FT_RESOURCE_STATE_UNDEFINED;
			barriers->image_barriers[ a ].new_state =
			    FT_RESOURCE_STATE_DEPTH_STENCIL_WRITE;
		}
	}
}

static void
rg_build_render_passes( struct RenderGraph* graph )
{
	if ( graph->physical_passes )
	{
		free( graph->physical_passes );
	}

	graph->physical_passes     = calloc( graph->render_pass_count,
                                     sizeof( struct RenderPassBeginInfo ) );
	graph->physical_pass_count = graph->render_pass_count;

	for ( u32 p = 0; p < graph->physical_pass_count; ++p )
	{
		struct RenderPass*          pass = &graph->render_passes[ p ];
		struct RenderPassBeginInfo* info = &graph->physical_passes[ p ];

		info->color_attachment_count = pass->color_attachment_count;
		info->width                  = graph->swapchain_image_width;
		info->height                 = graph->swapchain_image_height;

		for ( u32 i = 0; i < info->color_attachment_count; ++i )
		{
			struct AttachmentInfo* att = &info->color_attachments[ i ];

			att->image = graph->physical_images[ pass->color_attachments[ i ] ];

			if ( pass->clear_color( i, &att->clear_value.color ) )
			{
				att->load_op = FT_ATTACHMENT_LOAD_OP_CLEAR;
			}
			else
			{
				att->load_op = FT_ATTACHMENT_LOAD_OP_DONT_CARE;
			}
		}

		if ( pass->has_depth_stencil )
		{
			struct AttachmentInfo* att = &info->depth_attachment;
			att->image =
			    graph->physical_images[ pass->depth_stencil_attachment ];

			if ( pass->clear_depth_stencil( &att->clear_value.depth_stencil ) )
			{
				att->load_op = FT_ATTACHMENT_LOAD_OP_CLEAR;
			}
			else
			{
				att->load_op = FT_ATTACHMENT_LOAD_OP_DONT_CARE;
			}
		}

		pass->physical_pass_index = p;
		pass->create( graph->device, pass->user_data );
	}
}

void
rg_build( struct RenderGraph* graph )
{
	rg_create_physical_images( graph );
	rg_build_pass_barriers( graph );
	rg_build_render_passes( graph );
}

void
rg_set_swapchain_dimensions( struct RenderGraph* graph, u32 width, u32 height )
{
	graph->swapchain_image_width  = width;
	graph->swapchain_image_height = height;
}

void
rg_setup_attachments( struct RenderGraph* graph, struct Image* image )
{
	graph->swapchain_image = image;

	if ( graph->render_pass_count != 0 )
	{
		struct RenderPass* pass =
		    &graph->render_passes[ graph->swapchain_pass_index ];
		struct RenderPassBeginInfo* info =
		    &graph->physical_passes[ pass->physical_pass_index ];
		struct PassBarriers* barriers =
		    &graph->pass_barriers[ pass->physical_pass_index ];

		for ( u32 i = 0; i < info->color_attachment_count; ++i )
		{
			struct AttachmentInfo* att = &info->color_attachments[ i ];
			if ( pass->color_attachments[ i ] == graph->swapchain_image_index )
			{
				att->image                          = image;
				barriers->image_barriers[ i ].image = image;
			}
		}
	}
}

void
rg_execute( struct CommandBuffer* cmd, struct RenderGraph* graph )
{
	for ( u32 p = 0; p < graph->physical_pass_count; ++p )
	{
		struct RenderPass*          pass = &graph->render_passes[ p ];
		struct RenderPassBeginInfo* info =
		    &graph->physical_passes[ pass->physical_pass_index ];
		struct PassBarriers* barriers =
		    &graph->pass_barriers[ pass->physical_pass_index ];

		cmd_barrier( cmd,
		             0,
		             NULL,
		             0,
		             NULL,
		             barriers->image_barrier_count,
		             barriers->image_barriers );

		cmd_begin_render_pass( cmd, info );
		pass->execute( graph->device, cmd, pass->user_data );
		cmd_end_render_pass( cmd );
	}

	struct ImageBarrier barrier = {
	    .image     = graph->swapchain_image,
	    .old_state = graph->physical_pass_count
	                     ? FT_RESOURCE_STATE_COLOR_ATTACHMENT
	                     : FT_RESOURCE_STATE_UNDEFINED,
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

	*pass = ( struct RenderPass ) {
	    .name                = name,
	    .graph               = graph,
	    .create              = default_create,
	    .execute             = default_execute,
	    .destroy             = default_destroy,
	    .clear_color         = default_clear_color,
	    .clear_depth_stencil = default_clear_depth_stencil,
	};

	*p = pass;
}

void
rg_add_color_output( struct RenderPass*      pass,
                     const char*             name,
                     const struct ImageInfo* info )
{
	struct ImageInfo* color_output;
	u32               index = rg_get_image( pass->graph, name, &color_output );
	*color_output           = *info;
	color_output->descriptor_type = FT_DESCRIPTOR_TYPE_COLOR_ATTACHMENT;
	pass->color_attachments[ pass->color_attachment_count++ ] = index;
}

void
rg_add_depth_stencil_output( struct RenderPass*      pass,
                             const char*             name,
                             const struct ImageInfo* info )
{
	struct ImageInfo* depth_stencil_output;
	u32 index = rg_get_image( pass->graph, name, &depth_stencil_output );
	*depth_stencil_output = *info;
	depth_stencil_output->descriptor_type =
	    FT_DESCRIPTOR_TYPE_DEPTH_STENCIL_ATTACHMENT;
	pass->depth_stencil_attachment = index;
	pass->has_depth_stencil        = 1;
}

void
rg_set_user_data( struct RenderPass* pass, void* data )
{
	FT_ASSERT( pass );
	pass->user_data = data;
}

void
rg_set_pass_create_callback( struct RenderPass* pass, create_cb cb )
{
	FT_ASSERT( pass );
	FT_ASSERT( cb );
	pass->create = cb;
}

void
rg_set_pass_execute_callback( struct RenderPass* pass, execute_cb cb )
{
	FT_ASSERT( pass );
	FT_ASSERT( cb );
	pass->execute = cb;
}

void
rg_set_pass_destroy_callback( struct RenderPass* pass, destroy_cb cb )
{
	FT_ASSERT( pass );
	FT_ASSERT( cb );
	pass->destroy = cb;
}

void
rg_set_get_clear_color( struct RenderPass* pass, get_clear_color_cb cb )
{
	FT_ASSERT( pass );
	FT_ASSERT( cb );
	pass->clear_color = cb;
}

void
rg_set_get_clear_depth_stencil( struct RenderPass*         pass,
                                get_clear_depth_stencil_cb cb )
{
	FT_ASSERT( pass );
	FT_ASSERT( cb );
	pass->clear_depth_stencil = cb;
}
