#include <cgltf/cgltf.h>
#include <stdio.h>
#include <stb/stb_image.h>
#include "log/log.h"
#include "model_loader.h"

void*
read_file_binary( const char* filename, u64* size )
{
	FILE* file = fopen( filename, "rb" );
	FT_ASSERT( file && "failed to open file" );
	u64 res = fseek( file, 0, SEEK_END );
	FT_ASSERT( res == 0 );
	*size = ftell( file );
	res   = fseek( file, 0, SEEK_SET );
	FT_ASSERT( res == 0 );
	void* file_data = malloc( *size );
	fread( file_data, *size, 1, file );
	res = fclose( file );
	FT_ASSERT( res == 0 );
	return file_data;
}

void
free_file_data( void* data )
{
	free( data );
}

static struct ModelTexture
load_image_from_cgltf_image( cgltf_image* cgltf_image, const char* filename )
{
	struct ModelTexture image = { 0 };

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
			u32 i = 0;
			while ( ( cgltf_image->uri[ i ] != ',' ) &&
			        ( cgltf_image->uri[ i ] != 0 ) )
				i++;

			if ( cgltf_image->uri[ i ] == 0 )
			{
				FT_WARN( "wrong image uri" );
			}
			else
			{
				u32   base64_size = ( u32 ) strlen( cgltf_image->uri + i + 1 );
				u32   out_size    = 3 * ( base64_size / 4 );
				void* data        = NULL;

				cgltf_options options = { 0 };
				cgltf_result  result =
				    cgltf_load_buffer_base64( &options,
				                              out_size,
				                              cgltf_image->uri + i + 1,
				                              &data );

				if ( result == cgltf_result_success )
				{
					i32 ch;
					i32 w, h;
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
			i32  ch;
			i32  w, h;
			char path[ 100 ];
			strcpy( path, filename );
			u32 i          = 0;
			u32 last_slash = 0;
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
		u8* data   = malloc( cgltf_image->buffer_view->size );
		u64 offset = cgltf_image->buffer_view->offset;
		u64 stride = cgltf_image->buffer_view->stride
		                 ? cgltf_image->buffer_view->stride
		                 : 1;

		// Copy buffer data to memory for loading
		for ( u32 i = 0; i < cgltf_image->buffer_view->size; i++ )
		{
			data[ i ] =
			    ( ( u8* ) cgltf_image->buffer_view->buffer->data )[ offset ];
			offset += stride;
		}

		if ( ( strcmp( cgltf_image->mime_type, "image\\/png" ) == 0 ) ||
		     ( strcmp( cgltf_image->mime_type, "image/png" ) == 0 ) )
		{
			i32 ch;
			i32 w, h;
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
			i32 ch;
			i32 w, h;
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

void
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

void
process_gltf_node( cgltf_node* node, struct Mesh* mesh, const char* filename )
{
	cgltf_mesh* gltf_mesh = node->mesh;

	if ( mesh != NULL )
	{
		f32 node_to_world[ 16 ];
		cgltf_node_transform_world( node, node_to_world );
		memcpy( mesh->world, node_to_world, sizeof( node_to_world ) );

		for ( cgltf_size p = 0; p < gltf_mesh->primitives_count; ++p )
		{
			cgltf_primitive* primitive = &gltf_mesh->primitives[ p ];

			for ( cgltf_size att = 0; att < primitive->attributes_count; ++att )
			{
				cgltf_attribute* attribute      = &primitive->attributes[ att ];
				cgltf_accessor*  accessor       = attribute->data;
				cgltf_size       accessor_count = accessor->count;

				cgltf_size float_count =
				    cgltf_accessor_unpack_floats( accessor, NULL, 0 );
				f32* accessor_data = malloc( float_count * sizeof( f32 ) );
				cgltf_accessor_unpack_floats( accessor,
				                              accessor_data,
				                              float_count );

				mesh->vertex_count = accessor_count;

				cgltf_size component_count =
				    cgltf_num_components( accessor->type );

				if ( attribute->type == cgltf_attribute_type_position &&
				     attribute->index == 0 )
				{
					mesh->positions =
					    malloc( sizeof( f32 ) * accessor_count * 3 );

					for ( cgltf_size v = 0; v < accessor_count * 3; v += 3 )
					{
						gltf_read_float( accessor_data,
						                 component_count,
						                 v / 3,
						                 &mesh->positions[ v ],
						                 3 );
					}
				}
				else if ( attribute->type == cgltf_attribute_type_normal &&
				          attribute->index == 0 )
				{
					mesh->normals =
					    malloc( accessor_count * sizeof( f32 ) * 3 );

					for ( cgltf_size v = 0; v < accessor_count * 3; v += 3 )
					{
						gltf_read_float( accessor_data,
						                 component_count,
						                 v / 3,
						                 &mesh->normals[ v ],
						                 3 );
					}
				}
				else if ( attribute->type == cgltf_attribute_type_texcoord &&
				          attribute->index == 0 )
				{
					mesh->texcoords =
					    malloc( accessor_count * sizeof( f32 ) * 2 );

					for ( cgltf_size v = 0; v < accessor_count * 2; v += 2 )
					{
						gltf_read_float( accessor_data,
						                 component_count,
						                 v / 2,
						                 &mesh->texcoords[ v ],
						                 2 );
					}
				}

				free( accessor_data );
			}

			if ( primitive->indices != NULL )
			{
				cgltf_accessor* accessor = primitive->indices;

				mesh->index_count = accessor->count;
				if ( accessor->component_type == cgltf_component_type_r_16u )
				{
					mesh->indices = malloc( accessor->count * sizeof( u16 ) );
					u16* indices  = mesh->indices;
					for ( cgltf_size v = 0; v < accessor->count; ++v )
					{
						indices[ v ] = cgltf_accessor_read_index( accessor, v );
					}
				}
				else if ( accessor->component_type ==
				          cgltf_component_type_r_32u )
				{
					mesh->is_32bit_indices = 1;
					mesh->indices = malloc( accessor->count * sizeof( u32 ) );

					u32* indices = mesh->indices;
					for ( cgltf_size v = 0; v < accessor->count; ++v )
					{
						indices[ v ] = cgltf_accessor_read_index( accessor, v );
					}
				}
			}

			if ( primitive->material )
			{
				if ( primitive->material->pbr_metallic_roughness
				         .base_color_texture.texture )
				{
					load_image_from_cgltf_image(
					    primitive->material->pbr_metallic_roughness
					        .base_color_texture.texture->image,
					    filename );
				}
			}
		}

		for ( cgltf_size child_index = 0; child_index < node->children_count;
		      ++child_index )
		{
			FT_ASSERT( 0 && "child nodes not implemented" );
		}
	}
}

struct Model
load_gltf( const char* filename )
{
	struct Model model = { 0 };

	u64 data_size = 0;
	u8* file_data = read_file_binary( filename, &data_size );

	FT_ASSERT( file_data != NULL && "failed to load gltf file" );

	cgltf_options options = { 0 };
	cgltf_data*   data    = NULL;
	cgltf_result  result = cgltf_parse( &options, file_data, data_size, &data );

	if ( result == cgltf_result_success )
	{
		result = cgltf_load_buffers( &options, data, filename );

		if ( result == cgltf_result_success )
		{
			FT_ASSERT( data->scenes_count == 1 );
			for ( cgltf_size s = 0; s < data->scenes_count; ++s )
			{
				cgltf_scene* scene = &data->scenes[ s ];

				model.mesh_count = data->meshes_count;
				model.meshes =
				    calloc( model.mesh_count, sizeof( struct Mesh ) );

				for ( cgltf_size n = 0; n < scene->nodes_count; ++n )
				{
					cgltf_node* node = scene->nodes[ n ];

					process_gltf_node( node, &model.meshes[ n ], filename );
				}
			}
		}

		cgltf_free( data );
	}

	free_file_data( file_data );

	return model;
}

void
free_mesh( struct Mesh* mesh )
{
	if ( mesh->positions )
	{
		free( mesh->positions );
	}

	if ( mesh->texcoords )
	{
		free( mesh->texcoords );
	}

	if ( mesh->normals )
	{
		free( mesh->normals );
	}

	if ( mesh->tangents )
	{
		free( mesh->tangents );
	}

	if ( mesh->indices )
	{
		free( mesh->indices );
	}
}

void
free_model( struct Model* model )
{
	for ( u32 i = 0; i < model->mesh_count; ++i )
	{
		free_mesh( &model->meshes[ i ] );
	}

	if ( model->meshes )
	{
		free( model->meshes );
	}
}
