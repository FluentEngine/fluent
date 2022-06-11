#include "render_graph_private.h"
#include "vulkan_graph.h"
#include "render_graph.h"

rg_destroy_fun               rg_destroy_impl;
rg_add_pass_fun              rg_add_pass_impl;
rg_add_color_output_fun      rg_add_color_output_impl;
rg_set_backbuffer_source_fun rg_set_backbuffer_source_impl;
rg_build_fun                 rg_build_impl;
rg_setup_attachments_fun     rg_setup_attachments_impl;
rg_execute_fun               rg_execute_impl;

static b32
compare_name_to_index( const void* a, const void* b, void* udata )
{
	const struct NameToIndex* na = a;
	const struct NameToIndex* nb = b;
	return strcmp(na->name, nb->name);
}

static u64
hash_name_to_index( const void* item, u64 seed0, u64 seed1 )
{
	return 0;
}

struct hashmap* create_name_to_index_map()
{
	return  hashmap_new( sizeof( struct NameToIndex ),
	                                       0,
	                                       0,
	                                       0,
	                                       hash_name_to_index,
	                                       compare_name_to_index,
	                                       NULL,
	                                       NULL );
}

void
rg_create( const struct Device* device, struct RenderGraph** graph )
{
	FT_ASSERT( device );
	FT_ASSERT( graph );

	switch ( device->api )
	{
	case FT_RENDERER_API_VULKAN:
	{
		vk_rg_create( device, graph );
		break;
	}
	default:
	{
		FT_ASSERT( 0 && "unimplemented api" );
	}
	}
}

void
rg_destroy( struct RenderGraph* graph )
{
	FT_ASSERT( graph );
	rg_destroy_impl( graph );
}

struct RenderGraphPass*
rg_add_pass( struct RenderGraph* graph, const char* pass_name )
{
	FT_ASSERT( graph );
	FT_ASSERT( pass_name );
	return rg_add_pass_impl( graph, pass_name );
}

void
rg_add_color_output( struct RenderGraphPass* pass,
                     const struct ImageInfo*       image_info,
                     const char*             name )
{
	FT_ASSERT( pass );
	FT_ASSERT( image_info );
	FT_ASSERT( name );
	rg_add_color_output_impl( pass, image_info, name );
}

void
rg_set_backbuffer_source( struct RenderGraph* graph, const char* name )
{
	FT_ASSERT( graph );
	rg_set_backbuffer_source_impl( graph, name );
}

void
rg_build( struct RenderGraph* graph )
{
	FT_ASSERT( graph );
	rg_build_impl( graph );
}

void
rg_setup_attachments( struct RenderGraph* graph, const struct Image* image )
{
	FT_ASSERT( graph );
	FT_ASSERT( image );
	rg_setup_attachments_impl( graph, image );
}

void
rg_execute( struct RenderGraph* graph, struct CommandBuffer* cmd )
{
	FT_ASSERT( graph );
	FT_ASSERT( cmd );
	rg_execute_impl( graph, cmd );
}
