#include <hashmap_c/hashmap_c.h>
#include "renderer_backend.h"
#include "render_graph.h"

struct name_to_index
{
	const char* name;
	uint32_t    index;
};

static int32_t
compare_name_to_index( const void* a, const void* b, void* udata )
{
	FT_UNUSED( udata );

	const struct name_to_index* na = a;
	const struct name_to_index* nb = b;

	return strcmp( na->name, nb->name );
}

static size_t
hash_name_to_index( const void* item, size_t seed0, size_t seed1 )
{
	FT_UNUSED( item );
	FT_UNUSED( seed0 );
	FT_UNUSED( seed1 );
	return 0;
}

FT_INLINE struct hashmap*
create_name_to_index_map()
{
	return hashmap_new( sizeof( struct name_to_index ),
	                    0,
	                    0,
	                    0,
	                    hash_name_to_index,
	                    compare_name_to_index,
	                    NULL,
	                    NULL );
}

struct ft_render_pass
{
	struct ft_render_graph* graph;
	const char*             name;
	uint32_t                physical_pass_index;

	void*                         user_data;
	bool                          created;
	ft_create_cb                  create;
	ft_execute_cb                 execute;
	ft_destroy_cb                 destroy;
	ft_get_clear_color_cb         clear_color;
	ft_get_clear_depth_stencil_cb clear_depth_stencil;

	uint32_t color_attachment_count;
	uint32_t color_attachments[ FT_MAX_ATTACHMENTS_COUNT ];
	bool     has_depth_stencil;
	uint32_t depth_stencil_attachment;
};

struct pass_barriers
{
	uint32_t                 image_barrier_count;
	struct ft_image_barrier* image_barriers;
};

struct ft_render_graph
{
	struct ft_device* device;

	uint32_t               render_pass_count;
	uint32_t               render_pass_capacity;
	struct ft_render_pass* render_passes;

	struct hashmap*       image_name_to_index;
	uint32_t              image_count;
	uint32_t              image_capacity;
	struct ft_image_info* images;

	struct ft_image* swapchain_image;
	uint32_t         swapchain_image_index;
	uint32_t         swapchain_pass_index;
	uint32_t         swapchain_image_width;
	uint32_t         swapchain_image_height;

	uint32_t                          physical_pass_count;
	struct ft_render_pass_begin_info* physical_passes;
	uint32_t                          physical_image_count;
	struct ft_image**                 physical_images;
	struct pass_barriers*             pass_barriers;
};

static void
default_create( const struct ft_device* device, void* user_data )
{
	FT_UNUSED( device );
	FT_UNUSED( user_data );
}

static void
default_execute( const struct ft_device*   device,
                 struct ft_command_buffer* cmd,
                 void*                     user_data )
{
	FT_UNUSED( device );
	FT_UNUSED( cmd );
	FT_UNUSED( user_data );
}

static void
default_destroy( const struct ft_device* device, void* user_data )
{
	FT_UNUSED( device );
	FT_UNUSED( user_data );
}

static bool
default_clear_color( uint32_t idx, ft_color_clear_value* color )
{
	FT_UNUSED( idx );
	FT_UNUSED( color );
	return false;
}

static bool
default_clear_depth_stencil(
    struct ft_depth_stencil_clear_value* depth_stencil )
{
	FT_UNUSED( depth_stencil );
	return false;
}

static uint32_t
rg_get_image( struct ft_render_graph* graph,
              const char*             name,
              struct ft_image_info**  info )
{
	struct name_to_index* it = hashmap_get( graph->image_name_to_index,
	                                        &( struct name_to_index ) {
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
		             &( struct name_to_index ) {
		                 .name  = name,
		                 .index = graph->image_count,
		             } );

		if ( graph->image_count == graph->image_capacity - 1 )
		{
			graph->image_capacity *= 2;
			graph->images = realloc( graph->images,
			                         graph->image_capacity *
			                             sizeof( struct ft_image_info ) );
		}

		memset( &graph->images[ graph->image_count ],
		        0,
		        sizeof( struct ft_image_info ) );

		*info = &graph->images[ graph->image_count ];

		return graph->image_count++;
	}
}

void
ft_rg_create( struct ft_device* device, struct ft_render_graph** p )
{
	FT_ASSERT( device );

	struct ft_render_graph* graph =
	    calloc( 1, sizeof( struct ft_render_graph ) );
	graph->device               = device;
	graph->render_pass_capacity = 1;
	graph->image_capacity       = 1;
	graph->image_name_to_index  = create_name_to_index_map();

	*p = graph;
}

FT_INLINE void
rg_cleanup( struct ft_render_graph* graph )
{
	if ( graph->physical_images )
	{
		for ( uint32_t i = 0; i < graph->physical_image_count; ++i )
		{
			if ( i != graph->swapchain_image_index )
			{
				ft_destroy_image( graph->device, graph->physical_images[ i ] );
			}
		}
		free( graph->physical_images );
	}

	if ( graph->pass_barriers )
	{
		for ( uint32_t p = 0; p < graph->render_pass_count; ++p )
		{
			ft_safe_free( graph->pass_barriers[ p ].image_barriers );
		}
		free( graph->pass_barriers );
	}

	for ( uint32_t i = 0; i < graph->render_pass_count; ++i )
	{
		struct ft_render_pass* pass = &graph->render_passes[ i ];
		if ( pass->created )
		{
			pass->destroy( graph->device, pass->user_data );
		}
	}

	ft_safe_free( graph->physical_passes );
}

void
ft_rg_destroy( struct ft_render_graph* graph )
{
	rg_cleanup( graph );
	hashmap_free( graph->image_name_to_index );
	ft_safe_free( graph->images );
	ft_safe_free( graph->render_passes );
	free( graph );
}

void
ft_rg_set_backbuffer_source( struct ft_render_graph* graph, const char* name )
{
	struct name_to_index* it     = hashmap_get( graph->image_name_to_index,
                                            &( struct name_to_index ) {
	                                                .name = name,
                                            } );
	graph->swapchain_image_index = it->index;
}

FT_INLINE void
ft_rg_create_physical_images( struct ft_render_graph* graph )
{
	graph->physical_image_count = graph->image_count;
	graph->physical_images =
	    calloc( graph->physical_image_count, sizeof( struct ft_image* ) );

	for ( uint32_t p = 0; p < graph->render_pass_count; ++p )
	{
		struct ft_render_pass* pass = &graph->render_passes[ p ];
		for ( uint32_t i = 0; i < pass->color_attachment_count; ++i )
		{
			if ( pass->color_attachments[ i ] == graph->swapchain_image_index )
			{
				graph->swapchain_pass_index = p;
			}
			else
			{
				graph->images[ pass->color_attachments[ i ] ].width =
				    graph->swapchain_image_width;
				graph->images[ pass->color_attachments[ i ] ].height =
				    graph->swapchain_image_height;

				ft_create_image(
				    graph->device,
				    &graph->images[ pass->color_attachments[ i ] ],
				    &graph->physical_images[ pass->color_attachments[ i ] ] );
			}
		}

		if ( pass->has_depth_stencil )
		{
			graph->images[ pass->depth_stencil_attachment ].width =
			    graph->swapchain_image_width;
			graph->images[ pass->depth_stencil_attachment ].height =
			    graph->swapchain_image_height;

			ft_create_image(
			    graph->device,
			    &graph->images[ pass->depth_stencil_attachment ],
			    &graph->physical_images[ pass->depth_stencil_attachment ] );
		}
	}
}

static void
ft_rg_build_pass_barriers( struct ft_render_graph* graph )
{
	if ( graph->render_pass_count == 0 )
	{
		return;
	}

	graph->pass_barriers =
	    calloc( graph->render_pass_count, sizeof( struct pass_barriers ) );

	for ( uint32_t p = 0; p < graph->render_pass_count; ++p )
	{
		struct ft_render_pass* pass     = &graph->render_passes[ p ];
		struct pass_barriers*  barriers = &graph->pass_barriers[ p ];
		barriers->image_barrier_count =
		    pass->color_attachment_count + pass->has_depth_stencil;

		if ( barriers->image_barrier_count == 0 )
		{
			continue;
		}

		barriers->image_barriers = calloc( barriers->image_barrier_count,
		                                   sizeof( struct ft_image_barrier ) );
		for ( uint32_t a = 0; a < pass->color_attachment_count; ++a )
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
			uint32_t a = pass->color_attachment_count;
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
ft_rg_build_render_passes( struct ft_render_graph* graph )
{
	graph->physical_passes =
	    calloc( graph->render_pass_count,
	            sizeof( struct ft_render_pass_begin_info ) );
	graph->physical_pass_count = graph->render_pass_count;

	for ( uint32_t p = 0; p < graph->physical_pass_count; ++p )
	{
		struct ft_render_pass*            pass = &graph->render_passes[ p ];
		struct ft_render_pass_begin_info* info = &graph->physical_passes[ p ];

		info->color_attachment_count = pass->color_attachment_count;
		info->width                  = graph->swapchain_image_width;
		info->height                 = graph->swapchain_image_height;

		for ( uint32_t i = 0; i < info->color_attachment_count; ++i )
		{
			struct ft_attachment_info* att = &info->color_attachments[ i ];

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
			struct ft_attachment_info* att = &info->depth_attachment;
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
		pass->created = 1;
	}
}

void
ft_rg_build( struct ft_render_graph* graph )
{
	rg_cleanup( graph );
	ft_rg_create_physical_images( graph );
	ft_rg_build_pass_barriers( graph );
	ft_rg_build_render_passes( graph );
}

void
ft_rg_set_swapchain_dimensions( struct ft_render_graph* graph,
                                uint32_t                width,
                                uint32_t                height )
{
	graph->swapchain_image_width  = width;
	graph->swapchain_image_height = height;
}

void
ft_rg_setup_attachments( struct ft_render_graph* graph, struct ft_image* image )
{
	graph->swapchain_image = image;

	if ( graph->render_pass_count != 0 )
	{
		struct ft_render_pass* pass =
		    &graph->render_passes[ graph->swapchain_pass_index ];
		struct ft_render_pass_begin_info* info =
		    &graph->physical_passes[ pass->physical_pass_index ];
		struct pass_barriers* barriers =
		    &graph->pass_barriers[ pass->physical_pass_index ];

		for ( uint32_t i = 0; i < info->color_attachment_count; ++i )
		{
			struct ft_attachment_info* att = &info->color_attachments[ i ];
			if ( pass->color_attachments[ i ] == graph->swapchain_image_index )
			{
				att->image                          = image;
				barriers->image_barriers[ i ].image = image;
			}
		}
	}
}

void
ft_rg_execute( struct ft_command_buffer* cmd, struct ft_render_graph* graph )
{
	for ( uint32_t p = 0; p < graph->physical_pass_count; ++p )
	{
		struct ft_render_pass*            pass = &graph->render_passes[ p ];
		struct ft_render_pass_begin_info* info =
		    &graph->physical_passes[ pass->physical_pass_index ];
		struct pass_barriers* barriers =
		    &graph->pass_barriers[ pass->physical_pass_index ];

		ft_cmd_barrier( cmd,
		                0,
		                NULL,
		                0,
		                NULL,
		                barriers->image_barrier_count,
		                barriers->image_barriers );

		ft_cmd_begin_render_pass( cmd, info );
		pass->execute( graph->device, cmd, pass->user_data );
		ft_cmd_end_render_pass( cmd );
	}

	struct ft_image_barrier barrier = {
	    .image     = graph->swapchain_image,
	    .old_state = graph->physical_pass_count
	                     ? FT_RESOURCE_STATE_COLOR_ATTACHMENT
	                     : FT_RESOURCE_STATE_UNDEFINED,
	    .new_state = FT_RESOURCE_STATE_PRESENT,
	};

	ft_cmd_barrier( cmd, 0, NULL, 0, NULL, 1, &barrier );
}

void
ft_rg_add_pass( struct ft_render_graph* graph,
                const char*             name,
                struct ft_render_pass** p )
{
	if ( graph->render_pass_count == ( graph->render_pass_capacity - 1 ) )
	{
		graph->render_pass_capacity *= 2;
		graph->render_passes = realloc( graph->render_passes,
		                                graph->render_pass_capacity *
		                                    sizeof( struct ft_render_pass ) );
	}

	struct ft_render_pass* pass =
	    &graph->render_passes[ graph->render_pass_count++ ];

	memset( pass, 0, sizeof( struct ft_render_pass ) );

	*pass = ( struct ft_render_pass ) {
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
ft_rg_add_color_output( struct ft_render_pass*      pass,
                        const char*                 name,
                        const struct ft_image_info* info )
{
	struct ft_image_info* color_output;
	uint32_t index = rg_get_image( pass->graph, name, &color_output );
	*color_output  = *info;
	color_output->descriptor_type = FT_DESCRIPTOR_TYPE_COLOR_ATTACHMENT;
	pass->color_attachments[ pass->color_attachment_count++ ] = index;
}

void
ft_rg_add_depth_stencil_output( struct ft_render_pass*      pass,
                                const char*                 name,
                                const struct ft_image_info* info )
{
	struct ft_image_info* depth_stencil_output;
	uint32_t index = rg_get_image( pass->graph, name, &depth_stencil_output );
	*depth_stencil_output = *info;
	depth_stencil_output->descriptor_type =
	    FT_DESCRIPTOR_TYPE_DEPTH_STENCIL_ATTACHMENT;
	pass->depth_stencil_attachment = index;
	pass->has_depth_stencil        = 1;
}

void
ft_rg_set_user_data( struct ft_render_pass* pass, void* data )
{
	FT_ASSERT( pass );
	pass->user_data = data;
}

void
ft_rg_set_pass_create_callback( struct ft_render_pass* pass, ft_create_cb cb )
{
	FT_ASSERT( pass );
	FT_ASSERT( cb );
	pass->create = cb;
}

void
ft_rg_set_pass_execute_callback( struct ft_render_pass* pass, ft_execute_cb cb )
{
	FT_ASSERT( pass );
	FT_ASSERT( cb );
	pass->execute = cb;
}

void
ft_rg_set_pass_destroy_callback( struct ft_render_pass* pass, ft_destroy_cb cb )
{
	FT_ASSERT( pass );
	FT_ASSERT( cb );
	pass->destroy = cb;
}

void
ft_rg_set_get_clear_color( struct ft_render_pass* pass,
                           ft_get_clear_color_cb  cb )
{
	FT_ASSERT( pass );
	FT_ASSERT( cb );
	pass->clear_color = cb;
}

void
ft_rg_set_get_clear_depth_stencil( struct ft_render_pass*        pass,
                                   ft_get_clear_depth_stencil_cb cb )
{
	FT_ASSERT( pass );
	FT_ASSERT( cb );
	pass->clear_depth_stencil = cb;
}
