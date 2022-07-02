#include <stdio.h>
#include <string.h>
#include <cgltf/cgltf.h>
#include <stb/stb_image.h>
#include <hashmap_c/hashmap_c.h>
#include "log/log.h"
#include "fs/fs.h"
#include "model_loader.h"

struct node_map_item
{
	cgltf_node* node;
	uint32_t    index;
};

static int
compare_node_map_item( const void* a, const void* b, void* udata )
{
	FT_UNUSED( udata );

	const struct node_map_item* nma = a;
	const struct node_map_item* nmb = b;

	return nma->node == nmb->node;
}

static uint64_t
hash_node_map_item( const void* item, uint64_t seed0, uint64_t seed1 )
{
	FT_UNUSED( item );
	FT_UNUSED( seed0 );
	FT_UNUSED( seed1 );
	// TODO: hash function
	return 0;
}

FT_INLINE struct ft_model_texture
load_image_from_cgltf_image( cgltf_image* cgltf_image, const char* filename )
{
	struct ft_model_texture image = { 0 };

	if ( cgltf_image->uri !=
	     NULL ) // Check if image data is provided as a uri (base64 or path)
	{
		if ( ( strlen( cgltf_image->uri ) > 5 ) &&
		     ( cgltf_image->uri[ 0 ] == 'd' ) &&
		     ( cgltf_image->uri[ 1 ] == 'a' ) &&
		     ( cgltf_image->uri[ 2 ] == 't' ) &&
		     ( cgltf_image->uri[ 3 ] == 'a' ) &&
		     ( cgltf_image->uri[ 4 ] == ':' ) )
		{
			uint32_t i = 0;
			while ( ( cgltf_image->uri[ i ] != ',' ) &&
			        ( cgltf_image->uri[ i ] != 0 ) )
				i++;

			if ( cgltf_image->uri[ i ] == 0 )
			{
				FT_WARN( "wrong image uri" );
			}
			else
			{
				uint32_t base64_size =
				    ( uint32_t ) strlen( cgltf_image->uri + i + 1 );
				uint32_t out_size = 3 * ( base64_size / 4 );
				void*    data     = NULL;

				cgltf_options options = { 0 };
				cgltf_result  result =
				    cgltf_load_buffer_base64( &options,
				                              out_size,
				                              cgltf_image->uri + i + 1,
				                              &data );

				if ( result == cgltf_result_success )
				{
					int32_t ch;
					int32_t w, h;
					image.data   = stbi_loadf_from_memory( data,
                                                         out_size,
                                                         &w,
                                                         &h,
                                                         &ch,
                                                         4 );
					image.width  = w;
					image.height = h;
					cgltf_free( ( cgltf_data* ) data );
				}
			}
		}
		else
		{
			int32_t ch;
			int32_t w, h;
			char    path[ 100 ];
			strcpy( path, filename );
			uint32_t i          = 0;
			uint32_t last_slash = 0;
			while ( i < 100 && path[ i ] != '\0' )
			{
				i++;
				if ( path[ i ] == '/' )
					last_slash = i;
			};
			path[ last_slash + 1 ] = '\0';
			strcat( path, cgltf_image->uri );
			FILE* f      = fopen( path, "r" );
			image.data   = stbi_load_from_file( f, &w, &h, &ch, 4 );
			image.width  = w;
			image.height = h;
		}
	}
	else if ( cgltf_image->buffer_view->buffer->data !=
	          NULL ) // Check if image is provided as data buffer
	{
		uint8_t* data   = malloc( cgltf_image->buffer_view->size );
		uint64_t offset = cgltf_image->buffer_view->offset;
		uint64_t stride = cgltf_image->buffer_view->stride
		                      ? cgltf_image->buffer_view->stride
		                      : 1;

		// Copy buffer data to memory for loading
		for ( uint32_t i = 0; i < cgltf_image->buffer_view->size; i++ )
		{
			data[ i ] =
			    ( ( uint8_t* )
			          cgltf_image->buffer_view->buffer->data )[ offset ];
			offset += stride;
		}

		if ( ( strcmp( cgltf_image->mime_type, "image\\/png" ) == 0 ) ||
		     ( strcmp( cgltf_image->mime_type, "image/png" ) == 0 ) )
		{
			int32_t ch;
			int32_t w, h;
			image.data   = stbi_loadf_from_memory( data,
                                                 cgltf_image->buffer_view->size,
                                                 &w,
                                                 &h,
                                                 &ch,
                                                 4 );
			image.width  = w;
			image.height = h;
		}
		else if ( ( strcmp( cgltf_image->mime_type, "image\\/jpeg" ) == 0 ) ||
		          ( strcmp( cgltf_image->mime_type, "image/jpeg" ) == 0 ) )
		{
			int32_t ch;
			int32_t w, h;
			image.data   = stbi_loadf_from_memory( data,
                                                 cgltf_image->buffer_view->size,
                                                 &w,
                                                 &h,
                                                 &ch,
                                                 4 );
			image.width  = w;
			image.height = h;
		}
		else
		{
			FT_WARN( "unknown image mime type" );
		}

		free( data );
	}

	return image;
}

FT_INLINE void
gltf_read_float( const float* accessor_data,
                 cgltf_size   accessor_num_components,
                 cgltf_size   index,
                 cgltf_float* out,
                 cgltf_size   out_element_size )
{
	const float* input = &accessor_data[ accessor_num_components * index ];

	for ( cgltf_size ii = 0; ii < out_element_size; ++ii )
	{
		out[ ii ] = ( ii < accessor_num_components ) ? input[ ii ] : 0.0f;
	}
}

FT_INLINE float**
get_mesh_attribute( struct ft_mesh* mesh, cgltf_attribute_type type )
{
	switch ( type )
	{
	case cgltf_attribute_type_position:
	{
		return &mesh->positions;
	}
	case cgltf_attribute_type_normal:
	{
		return &mesh->normals;
	}
	case cgltf_attribute_type_texcoord:
	{
		return &mesh->texcoords;
	}
	case cgltf_attribute_type_tangent:
	{
		return &mesh->tangents;
	}
	case cgltf_attribute_type_joints:
	{
		return &mesh->joints;
	}
	case cgltf_attribute_type_weights:
	{
		return &mesh->weights;
	}
	case cgltf_attribute_type_color:
	{
		FT_WARN( "gltf color attribute are not supported" );
		return NULL;
	}
	default:
	{
		FT_WARN( "invalid gltf attribute" );
		return NULL;
	}
	}
}

FT_INLINE void
read_attribute( struct ft_mesh*  mesh,
                cgltf_attribute* attribute,
                float*           accessor_data,
                cgltf_size       accessor_count,
                cgltf_size       component_count )
{
	if ( attribute->index != 0 )
	{
		return;
	}

	float** vertices = get_mesh_attribute( mesh, attribute->type );

	if ( vertices == NULL )
	{
		return;
	}

	*vertices =
	    realloc( *vertices,
	             ( mesh->vertex_count * component_count ) * sizeof( float ) );

	for ( cgltf_size v = 0; v < accessor_count * component_count;
	      v += component_count )
	{
		gltf_read_float( accessor_data,
		                 component_count,
		                 v / component_count,
		                 &( ( *vertices )[ v ] ),
		                 component_count );
	}
}

static uint32_t
process_gltf_node( struct hashmap* node_map,
                   uint32_t        index,
                   cgltf_node*     node,
                   struct ft_mesh* meshes,
                   const char*     filename )
{
	uint32_t    mesh_index = 0;
	cgltf_mesh* gltf_mesh  = node->mesh;

	if ( gltf_mesh != NULL )
	{
		hashmap_set( node_map,
		             &( struct node_map_item ) {
		                 .node  = node,
		                 .index = index,
		             } );

		for ( cgltf_size p = 0; p < gltf_mesh->primitives_count; ++p )
		{
			struct ft_mesh* mesh = &meshes[ p ];
			float           node_to_world[ 16 ];
			memset( node_to_world, 0, sizeof( node_to_world ) );
			cgltf_node_transform_world( node, node_to_world );
			memcpy( mesh->world, node_to_world, sizeof( node_to_world ) );

			cgltf_primitive* primitive = &gltf_mesh->primitives[ p ];

			for ( cgltf_size att = 0; att < primitive->attributes_count; ++att )
			{
				cgltf_attribute* attribute      = &primitive->attributes[ att ];
				cgltf_accessor*  accessor       = attribute->data;
				cgltf_size       accessor_count = accessor->count;

				cgltf_size float_count =
				    cgltf_accessor_unpack_floats( accessor, NULL, 0 );
				float* accessor_data = malloc( float_count * sizeof( float ) );
				cgltf_accessor_unpack_floats( accessor,
				                              accessor_data,
				                              float_count );

				mesh->vertex_count = ( att == 0 )
				                         ? mesh->vertex_count + accessor_count
				                         : mesh->vertex_count;

				read_attribute( mesh,
				                attribute,
				                accessor_data,
				                accessor_count,
				                cgltf_num_components( accessor->type ) );

				free( accessor_data );
			}

			if ( primitive->indices != NULL )
			{
				cgltf_accessor* accessor = primitive->indices;

				mesh->index_count += accessor->count;
				if ( accessor->component_type == cgltf_component_type_r_16u )
				{
					mesh->indices =
					    realloc( mesh->indices,
					             mesh->index_count * sizeof( uint16_t ) );
					uint16_t* indices = mesh->indices;
					for ( cgltf_size v = 0; v < accessor->count; ++v )
					{
						indices[ v ] = cgltf_accessor_read_index( accessor, v );
					}
				}
				else if ( accessor->component_type ==
				          cgltf_component_type_r_32u )
				{
					mesh->is_32bit_indices = 1;
					mesh->indices =
					    realloc( mesh->indices,
					             mesh->index_count * sizeof( uint32_t ) );

					uint32_t* indices = mesh->indices;
					for ( cgltf_size v = 0; v < accessor->count; ++v )
					{
						indices[ v ] = cgltf_accessor_read_index( accessor, v );
					}
				}
			}
		}
		mesh_index = gltf_mesh->primitives_count;
	}

	for ( cgltf_size child_index = 0; child_index < node->children_count;
	      ++child_index )
	{
		mesh_index += process_gltf_node( node_map,
		                                 index,
		                                 node->children[ child_index ],
		                                 &meshes[ mesh_index ],
		                                 filename );
	}

	return mesh_index;
}

static void
read_animation_samplers( const cgltf_animation_sampler* src,
                         struct ft_animation_sampler*   dst )
{
	const cgltf_accessor* timeline_accessor = src->input;
	const uint8_t* timeline_blob = timeline_accessor->buffer_view->buffer->data;
	const float*   timeline_floats =
	    ( const float* ) ( timeline_blob + timeline_accessor->offset +
	                       timeline_accessor->buffer_view->offset );

	dst->frame_count = timeline_accessor->count;

	FT_ASSERT( dst->times == NULL );
	dst->times = calloc( dst->frame_count, sizeof( float ) );

	for ( uint32_t i = 0; i < dst->frame_count; ++i )
	{
		dst->times[ i ] = timeline_floats[ i ];
	}

	// TODO: sort times

	const cgltf_accessor* values_accessor = src->output;

	FT_ASSERT( dst->values == NULL );

	switch ( values_accessor->type )
	{
	case cgltf_type_scalar:
	{
		dst->values = calloc( dst->frame_count, sizeof( float ) );
		cgltf_accessor_unpack_floats( src->output,
		                              &dst->values[ 0 ],
		                              dst->frame_count );
		break;
	}
	case cgltf_type_vec3:
	{
		dst->values = calloc( dst->frame_count, 3 * sizeof( float ) );
		cgltf_accessor_unpack_floats( src->output,
		                              &dst->values[ 0 ],
		                              dst->frame_count * 3 );
		break;
	}
	case cgltf_type_vec4:
	{
		dst->values = calloc( dst->frame_count, 4 * sizeof( float ) );
		cgltf_accessor_unpack_floats( src->output,
		                              &dst->values[ 0 ],
		                              dst->frame_count * 4 );
		break;
	}
	default: return;
	}

	switch ( src->interpolation )
	{
	case cgltf_interpolation_type_step:
		dst->interpolation = FT_ANIMATION_INTERPOLATION_STEP;
		break;
	case cgltf_interpolation_type_linear:
		dst->interpolation = FT_ANIMATION_INTERPOLATION_LINEAR;
		break;
	case cgltf_interpolation_type_cubic_spline:
		dst->interpolation = FT_ANIMATION_INTERPOLATION_SPLINE;
		break;
	default: break;
	}
}

static void
read_animation_channels( struct hashmap*        node_map,
                         const cgltf_animation* src,
                         struct ft_animation*   dst )
{
	cgltf_animation_channel* src_channels = src->channels;
	cgltf_animation_sampler* src_samplers = src->samplers;

	dst->channel_count = src->channels_count;
	dst->channels =
	    calloc( src->channels_count, sizeof( struct ft_animation_channel ) );

	for ( cgltf_size j = 0; j < src->channels_count; ++j )
	{
		const cgltf_animation_channel* src_channel = &src_channels[ j ];
		struct ft_animation_channel*   dst_channel = &dst->channels[ j ];
		dst_channel->sampler =
		    dst->samplers + ( src_channel->sampler - src_samplers );

		struct node_map_item* it =
		    hashmap_get( node_map,
		                 &( struct node_map_item ) {
		                     .node = src_channel->target_node->parent,
		                 } );

		if ( it != NULL )
		{
			dst_channel->target = it->index;
		}
		else
		{
			FT_WARN( "no scene root contains node %s"
			         "for animation %s"
			         "in channel %d",
			         src_channel->target_node->parent->name,
			         src->name,
			         j );
		}

		switch ( src_channel->target_path )
		{
		case cgltf_animation_path_type_translation:
			dst_channel->transform_type = FT_TRANSFORM_TYPE_TRANSLATION;
			break;
		case cgltf_animation_path_type_rotation:
			dst_channel->transform_type = FT_TRANSFORM_TYPE_ROTATION;
			break;
		case cgltf_animation_path_type_scale:
			dst_channel->transform_type = FT_TRANSFORM_TYPE_SCALE;
			break;
		case cgltf_animation_path_type_weights:
			dst_channel->transform_type = FT_TRANSFORM_TYPE_WEIGHTS;
			break;
		default: break;
		}
	}
}

struct ft_model
ft_load_gltf( const char* filename )
{
	struct ft_model model;
	memset( &model, 0, sizeof( struct ft_model ) );

	uint64_t data_size = 0;
	uint8_t* file_data = ft_read_file_binary( filename, &data_size );

	if ( file_data == NULL )
	{
		FT_WARN( "failed to read gltf file %s", filename );
		return model;
	}

	cgltf_options options = { 0 };
	cgltf_data*   data    = NULL;
	cgltf_result  result = cgltf_parse( &options, file_data, data_size, &data );

	if ( result == cgltf_result_success )
	{
		result = cgltf_load_buffers( &options, data, filename );

		if ( result == cgltf_result_success )
		{
			if ( data->scenes_count > 1 )
			{
				FT_WARN( "multiple scenes gltf not supported %s", filename );
				data->scenes_count = 1;
			}

			struct hashmap* node_map =
			    hashmap_new( sizeof( struct node_map_item ),
			                 0,
			                 0,
			                 0,
			                 hash_node_map_item,
			                 compare_node_map_item,
			                 NULL,
			                 NULL );

			for ( cgltf_size s = 0; s < data->scenes_count; ++s )
			{
				cgltf_scene* scene = &data->scenes[ s ];

				model.mesh_count = 0;

				for ( uint32_t m = 0; m < data->meshes_count; m++ )
					model.mesh_count += data->meshes[ m ].primitives_count;

				model.meshes =
				    calloc( model.mesh_count, sizeof( struct ft_mesh ) );

				uint32_t mesh_index = 0;
				for ( cgltf_size n = 0; n < scene->nodes_count; ++n )
				{
					cgltf_node* node = scene->nodes[ n ];

					mesh_index +=
					    process_gltf_node( node_map,
					                       n,
					                       node,
					                       &model.meshes[ mesh_index ],
					                       filename );
				}
			}

			model.animation_count = data->animations_count;
			model.animations =
			    calloc( model.animation_count, sizeof( struct ft_animation ) );

			for ( cgltf_size a = 0; a < model.animation_count; ++a )
			{
				cgltf_animation*         animation = &data->animations[ a ];
				cgltf_animation_sampler* samplers  = animation->samplers;

				model.animations[ a ].samplers =
				    calloc( animation->samplers_count,
				            sizeof( struct ft_animation_sampler ) );
				model.animations[ a ].sampler_count = animation->samplers_count;

				for ( cgltf_size s = 0; s < animation->samplers_count; ++s )
				{
					struct ft_animation_sampler* sampler =
					    &model.animations[ a ].samplers[ s ];

					read_animation_samplers( &samplers[ s ], sampler );

					if ( sampler->times[ sampler->frame_count - 1 ] >
					     model.animations[ a ].duration )
					{
						model.animations[ a ].duration =
						    sampler->times[ sampler->frame_count - 1 ];
					}
				}

				read_animation_channels( node_map,
				                         animation,
				                         &model.animations[ a ] );
			}

			hashmap_free( node_map );
		}
		else
		{
			FT_WARN( "failed to load gltf buffers %s", filename );
		}

		cgltf_free( data );
	}
	else
	{
		FT_WARN( "failed to parse gltf %s", filename );
	}

	ft_free_file_data( file_data );

	return model;
}

void
free_mesh( struct ft_mesh* mesh )
{
	ft_safe_free( mesh->positions );
	ft_safe_free( mesh->texcoords );
	ft_safe_free( mesh->normals );
	ft_safe_free( mesh->tangents );
	ft_safe_free( mesh->joints );
	ft_safe_free( mesh->indices );
}

FT_INLINE void
free_animation( struct ft_animation* animation )
{
	for ( uint32_t i = 0; i < animation->sampler_count; ++i )
	{
		struct ft_animation_sampler* sampler = &animation->samplers[ i ];
		ft_safe_free( sampler->times );
		ft_safe_free( sampler->values );
	}

	ft_safe_free( animation->samplers );
	ft_safe_free( animation->channels );
}

void
ft_free_gltf( struct ft_model* model )
{
	for ( uint32_t i = 0; i < model->animation_count; ++i )
	{
		free_animation( &model->animations[ i ] );
	}

	ft_safe_free( model->animations );

	for ( uint32_t i = 0; i < model->mesh_count; ++i )
	{
		free_mesh( &model->meshes[ i ] );
	}

	ft_safe_free( model->meshes );
}
