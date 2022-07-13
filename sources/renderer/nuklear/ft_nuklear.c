#define NK_IMPLEMENTATION
#include "ft_nuklear.h"

#include "shaders/shader_nuklear_frag.h"
#include "shaders/shader_nuklear_vert.h"
#include "shaders/shader_nuklear_frag.h"

#ifndef NK_FT_TEXT_MAX
#define NK_FT_TEXT_MAX 256
#endif

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_INDEX_BUFFER  128 * 1024

#define MAX( X, Y ) ( ( ( X ) > ( Y ) ) ? ( X ) : ( Y ) )

struct Mat4f
{
	float m[ 16 ];
};

struct nk_vulkan_adapter
{
	struct nk_buffer                 cmds;
	struct nk_draw_null_texture      null;
	struct ft_device                *device;
	struct ft_queue                 *queue;
	enum ft_format                   color_format;
	enum ft_format                   depth_format;
	struct ft_sampler               *font_tex;
	struct ft_image                 *font_image;
	struct ft_pipeline              *pipeline;
	struct ft_buffer                *vertex_buffer;
	struct ft_buffer                *index_buffer;
	struct ft_buffer                *uniform_buffer;
	struct ft_command_pool          *command_pool;
	struct ft_command_buffer        *command_buffer;
	struct ft_descriptor_set_layout *dsl;
	struct ft_descriptor_set        *set;
};

struct nk_ft_vertex
{
	float   position[ 2 ];
	float   uv[ 2 ];
	nk_byte col[ 4 ];
};

static struct nk_ft
{
	struct ft_wsi_info      *wsi;
	uint32_t                 width, height;
	uint32_t                 display_width, display_height;
	struct nk_vulkan_adapter adapter;
	struct nk_context        ctx;
	struct nk_font_atlas     atlas;
	struct nk_vec2           fb_scale;
	unsigned int             text[ NK_FT_TEXT_MAX ];
	int                      text_len;
	struct nk_vec2           scroll;
	double                   last_button_click;
	int                      is_double_click_down;
	struct nk_vec2           double_click_pos;
} ft;

static void
update_write_descriptor_sets( struct nk_vulkan_adapter *adapter )
{
	struct ft_buffer_descriptor buffer_descriptor = {
	    .buffer = adapter->uniform_buffer,
	    .offset = 0,
	    .range  = sizeof( struct Mat4f ),
	};

	struct ft_sampler_descriptor sampler_descriptor = {
	    .sampler = adapter->font_tex,
	};

	struct ft_image_descriptor image_descriptor = {
	    .image          = adapter->font_image,
	    .resource_state = FT_RESOURCE_STATE_SHADER_READ_ONLY };

	struct ft_descriptor_write descriptor_writes[ 3 ];
	memset( descriptor_writes, 0, sizeof( descriptor_writes ) );
	descriptor_writes[ 0 ].buffer_descriptors = &buffer_descriptor;
	descriptor_writes[ 0 ].descriptor_count   = 1;
	descriptor_writes[ 0 ].descriptor_name    = "ubo";

	descriptor_writes[ 1 ].sampler_descriptors = &sampler_descriptor;
	descriptor_writes[ 1 ].descriptor_name     = "u_sampler";
	descriptor_writes[ 1 ].descriptor_count    = 1;

	descriptor_writes[ 2 ].image_descriptors = &image_descriptor;
	descriptor_writes[ 2 ].descriptor_name   = "u_texture";
	descriptor_writes[ 2 ].descriptor_count  = 1;

	uint32_t descriptor_write_count = 1;
	if ( adapter->font_tex != NULL )
	{
		descriptor_write_count += 2;
	}
	ft_update_descriptor_set( adapter->device,
	                          adapter->set,
	                          descriptor_write_count,
	                          descriptor_writes );
}

static void
prepare_pipeline( struct nk_vulkan_adapter *adapter )
{
	struct ft_shader *shader;

	struct ft_shader_info shader_info = {
	    .vertex   = get_nuklear_vert_shader( adapter->device->api ),
	    .fragment = get_nuklear_frag_shader( adapter->device->api ),
	};
	ft_create_shader( adapter->device, &shader_info, &shader );

	ft_create_descriptor_set_layout( adapter->device, shader, &adapter->dsl );
	struct ft_descriptor_set_info set_info = {
	    .set                   = 0,
	    .descriptor_set_layout = adapter->dsl,
	};
	ft_create_descriptor_set( adapter->device, &set_info, &adapter->set );

	struct ft_pipeline_info pipeline_info = {
	    .type                          = FT_PIPELINE_TYPE_GRAPHICS,
	    .shader                        = shader,
	    .descriptor_set_layout         = adapter->dsl,
	    .topology                      = FT_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	    .rasterizer_info.polygon_mode  = FT_POLYGON_MODE_FILL,
	    .rasterizer_info.cull_mode     = FT_CULL_MODE_NONE,
	    .rasterizer_info.front_face    = FT_FRONT_FACE_CLOCKWISE,
	    .depth_state_info.depth_test   = 1,
	    .depth_state_info.depth_write  = 1,
	    .depth_state_info.compare_op   = FT_COMPARE_OP_ALWAYS,
	    .sample_count                  = 1,
	    .color_attachment_count        = 1,
	    .color_attachment_formats[ 0 ] = adapter->color_format,
	    .depth_stencil_format          = adapter->depth_format,
	    .blend_state_info.attachment_states[ 0 ] =
	        {
	            .src_factor       = FT_BLEND_FACTOR_SRC_ALPHA,
	            .dst_factor       = FT_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	            .op               = FT_BLEND_OP_ADD,
	            .src_alpha_factor = FT_BLEND_FACTOR_ONE,
	            .dst_alpha_factor = FT_BLEND_FACTOR_ZERO,
	            .alpha_op         = FT_BLEND_OP_ADD,
	        },
	    .vertex_layout =
	        {
	            .binding_info_count            = 1,
	            .binding_infos[ 0 ].binding    = 0,
	            .binding_infos[ 0 ].stride     = sizeof( struct nk_ft_vertex ),
	            .binding_infos[ 0 ].input_rate = FT_VERTEX_INPUT_RATE_VERTEX,
	            .attribute_info_count          = 3,
	            .attribute_infos[ 0 ].binding  = 0,
	            .attribute_infos[ 0 ].location = 0,
	            .attribute_infos[ 0 ].format   = FT_FORMAT_R32G32_SFLOAT,
	            .attribute_infos[ 0 ].offset =
	                NK_OFFSETOF( struct nk_ft_vertex, position ),
	            .attribute_infos[ 1 ].binding  = 0,
	            .attribute_infos[ 1 ].location = 1,
	            .attribute_infos[ 1 ].format   = FT_FORMAT_R32G32_SFLOAT,
	            .attribute_infos[ 1 ].offset =
	                NK_OFFSETOF( struct nk_ft_vertex, uv ),
	            .attribute_infos[ 2 ].binding  = 0,
	            .attribute_infos[ 2 ].location = 2,
	            .attribute_infos[ 2 ].format   = FT_FORMAT_R8G8B8A8_UINT,
	            .attribute_infos[ 2 ].offset =
	                NK_OFFSETOF( struct nk_ft_vertex, col ),
	        },
	};

	ft_create_pipeline( adapter->device, &pipeline_info, &adapter->pipeline );
	ft_destroy_shader( adapter->device, shader );
}

NK_API void
nk_ft_device_create( struct ft_device *device,
                     struct ft_queue  *graphics_queue,
                     enum ft_format    color_format,
                     enum ft_format    depth_format )
{
	struct nk_vulkan_adapter *adapter = &ft.adapter;
	nk_buffer_init_default( &adapter->cmds );
	adapter->device       = device;
	adapter->queue        = graphics_queue;
	adapter->color_format = color_format;
	adapter->depth_format = depth_format;

	struct ft_buffer_info buffer_info = { 0 };
	buffer_info.memory_usage          = FT_MEMORY_USAGE_CPU_TO_GPU;
	buffer_info.descriptor_type       = FT_DESCRIPTOR_TYPE_VERTEX_BUFFER;
	buffer_info.size                  = MAX_VERTEX_BUFFER;
	ft_create_buffer( adapter->device, &buffer_info, &adapter->vertex_buffer );
	buffer_info.descriptor_type = FT_DESCRIPTOR_TYPE_INDEX_BUFFER;
	buffer_info.size            = MAX_INDEX_BUFFER;
	ft_create_buffer( adapter->device, &buffer_info, &adapter->index_buffer );
	buffer_info.descriptor_type = FT_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	buffer_info.size            = sizeof( struct Mat4f );
	ft_create_buffer( adapter->device, &buffer_info, &adapter->uniform_buffer );

	prepare_pipeline( adapter );
}

NK_API void
nk_ft_device_destroy( void )
{
	struct nk_vulkan_adapter *adapter = &ft.adapter;
	nk_buffer_free( &adapter->cmds );
}

NK_API struct nk_context *
nk_ft_init( struct ft_wsi_info *wsi,
            struct ft_device   *device,
            struct ft_queue    *queue,
            enum ft_format      color_format,
            enum ft_format      depth_format )
{
	ft.wsi = wsi;

	nk_init_default( &ft.ctx, NULL );
	//	ft.ctx.clip.copy     = nk_ft_clipbard_copy;
	//	ft.ctx.clip.paste    = nk_ft_clipbard_paste;
	ft.ctx.clip.userdata = nk_handle_ptr( 0 );
	ft.last_button_click = 0;
	nk_ft_device_create( device, queue, color_format, depth_format );

	ft.is_double_click_down = nk_false;
	ft.double_click_pos     = nk_vec2( 0, 0 );

	return &ft.ctx;
}

NK_INTERN void
nk_ft_device_upload_atlas( const void *image, int width, int height )
{
	struct nk_vulkan_adapter *adapter = &ft.adapter;

	struct ft_image_info image_info = {
	    .width           = ( uint32_t ) width,
	    .height          = ( uint32_t ) height,
	    .depth           = 1,
	    .format          = FT_FORMAT_R8G8B8A8_UNORM,
	    .sample_count    = 1,
	    .layer_count     = 1,
	    .mip_levels      = 1,
	    .descriptor_type = FT_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
	};

	ft_create_image( adapter->device, &image_info, &adapter->font_image );

	struct ft_upload_image_info upload_info = {
	    .data      = image,
	    .width     = image_info.width,
	    .height    = image_info.height,
	    .mip_level = 0,
	};

	ft_upload_image( adapter->font_image, &upload_info );

	struct ft_sampler_info sampler_info = {
	    .mag_filter     = FT_FILTER_LINEAR,
	    .min_filter     = FT_FILTER_LINEAR,
	    .max_anisotropy = 1.0,
	    .mipmap_mode    = FT_SAMPLER_MIPMAP_MODE_LINEAR,
	    .address_mode_u = FT_SAMPLER_ADDRESS_MODE_REPEAT,
	    .address_mode_v = FT_SAMPLER_ADDRESS_MODE_REPEAT,
	    .address_mode_w = FT_SAMPLER_ADDRESS_MODE_REPEAT,
	    .mip_lod_bias   = 0.0f,
	    .compare_enable = 0,
	    .compare_op     = FT_COMPARE_OP_ALWAYS,
	    .min_lod        = 0.0f,
	    .max_lod        = 0.0f };

	ft_create_sampler( adapter->device, &sampler_info, &adapter->font_tex );
}

NK_API void
nk_ft_font_stash_begin( struct nk_font_atlas **atlas )
{
	nk_font_atlas_init_default( &ft.atlas );
	nk_font_atlas_begin( &ft.atlas );
	*atlas = &ft.atlas;
}

NK_API void
nk_ft_font_stash_end( void )
{
	struct nk_vulkan_adapter *dev = &ft.adapter;

	const void *image;
	int         w, h;
	image = nk_font_atlas_bake( &ft.atlas, &w, &h, NK_FONT_ATLAS_RGBA32 );
	nk_ft_device_upload_atlas( image, w, h );
	nk_font_atlas_end( &ft.atlas,
	                   nk_handle_ptr( dev->font_tex ),
	                   &ft.adapter.null );
	if ( ft.atlas.default_font )
	{
		nk_style_set_font( &ft.ctx, &ft.atlas.default_font->handle );
	}

	update_write_descriptor_sets( dev );
}

NK_API void
nk_ft_new_frame()
{
	struct nk_context *ctx = &ft.ctx;

	struct ft_wsi_info     *wsi = ft.wsi;
	const struct ft_window *win = wsi->window;
	wsi->get_window_size( win, &ft.width, &ft.height );
	wsi->get_framebuffer_size( win, &ft.display_width, &ft.display_height );

	ft.fb_scale.x = ( float ) ( ft.display_width / ft.width );
	ft.fb_scale.y = ( float ) ( ft.display_height / ft.height );

	nk_input_begin( ctx );

	nk_input_key( ctx, NK_KEY_DEL, ft_is_key_pressed( FT_KEY_DELETE ) );
	nk_input_key( ctx, NK_KEY_ENTER, ft_is_key_pressed( FT_KEY_ENTER ) );
	nk_input_key( ctx, NK_KEY_TAB, ft_is_key_pressed( FT_KEY_TAB ) );
	nk_input_key( ctx,
	              NK_KEY_BACKSPACE,
	              ft_is_key_pressed( FT_KEY_BACKSPACE ) );
	nk_input_key( ctx, NK_KEY_UP, ft_is_key_pressed( FT_KEY_UP ) );
	nk_input_key( ctx, NK_KEY_DOWN, ft_is_key_pressed( FT_KEY_DOWN ) );
	nk_input_key( ctx, NK_KEY_TEXT_START, ft_is_key_pressed( FT_KEY_HOME ) );
	nk_input_key( ctx, NK_KEY_TEXT_END, ft_is_key_pressed( FT_KEY_END ) );
	nk_input_key( ctx, NK_KEY_SCROLL_START, ft_is_key_pressed( FT_KEY_HOME ) );
	nk_input_key( ctx, NK_KEY_SCROLL_END, ft_is_key_pressed( FT_KEY_END ) );
	nk_input_key( ctx,
	              NK_KEY_SCROLL_DOWN,
	              ft_is_key_pressed( FT_KEY_PAGE_DOWN ) );
	nk_input_key( ctx, NK_KEY_SCROLL_UP, ft_is_key_pressed( FT_KEY_PAGE_UP ) );
	nk_input_key( ctx,
	              NK_KEY_SHIFT,
	              ft_is_key_pressed( FT_KEY_LEFT_SHIFT ) ||
	                  ft_is_key_pressed( FT_KEY_RIGHT_SHIFT ) );

	if ( ft_is_key_pressed( FT_KEY_LEFT_CONTROL ) ||
	     ft_is_key_pressed( FT_KEY_RIGHT_CONTROL ) )
	{
		nk_input_key( ctx, NK_KEY_COPY, ft_is_key_pressed( FT_KEY_C ) );
		nk_input_key( ctx, NK_KEY_PASTE, ft_is_key_pressed( FT_KEY_V ) );
		nk_input_key( ctx, NK_KEY_CUT, ft_is_key_pressed( FT_KEY_X ) );
		nk_input_key( ctx, NK_KEY_TEXT_UNDO, ft_is_key_pressed( FT_KEY_Z ) );
		nk_input_key( ctx, NK_KEY_TEXT_REDO, ft_is_key_pressed( FT_KEY_R ) );
		nk_input_key( ctx,
		              NK_KEY_TEXT_WORD_LEFT,
		              ft_is_key_pressed( FT_KEY_LEFT ) );
		nk_input_key( ctx,
		              NK_KEY_TEXT_WORD_RIGHT,
		              ft_is_key_pressed( FT_KEY_RIGHT ) );
		nk_input_key( ctx,
		              NK_KEY_TEXT_LINE_START,
		              ft_is_key_pressed( FT_KEY_B ) );
		nk_input_key( ctx,
		              NK_KEY_TEXT_LINE_END,
		              ft_is_key_pressed( FT_KEY_E ) );
	}
	else
	{
		nk_input_key( ctx, NK_KEY_LEFT, ft_is_key_pressed( FT_KEY_LEFT ) );
		nk_input_key( ctx, NK_KEY_RIGHT, ft_is_key_pressed( FT_KEY_RIGHT ) );
		nk_input_key( ctx, NK_KEY_COPY, 0 );
		nk_input_key( ctx, NK_KEY_PASTE, 0 );
		nk_input_key( ctx, NK_KEY_CUT, 0 );
		nk_input_key( ctx, NK_KEY_SHIFT, 0 );
	}

	int32_t x, y;
	ft_get_mouse_position( &x, &y );

	nk_input_motion( ctx, ( int ) x, ( int ) y );

	nk_input_button( ctx,
	                 NK_BUTTON_LEFT,
	                 ( int ) x,
	                 ( int ) y,
	                 ft_is_button_pressed( FT_BUTTON_LEFT ) );

	nk_input_button( ctx,
	                 NK_BUTTON_MIDDLE,
	                 x,
	                 y,
	                 ft_is_button_pressed( FT_BUTTON_MIDDLE ) );

	nk_input_button( ctx,
	                 NK_BUTTON_RIGHT,
	                 x,
	                 y,
	                 ft_is_button_pressed( FT_BUTTON_RIGHT ) );

	nk_input_button( ctx,
	                 NK_BUTTON_DOUBLE,
	                 ( int32_t ) ft.double_click_pos.x,
	                 ( int32_t ) ft.double_click_pos.y,
	                 ft.is_double_click_down );

	nk_input_scroll( ctx, ft.scroll );

	nk_input_end( &ft.ctx );
}

NK_API
void
nk_ft_render( const struct ft_command_buffer *cmd, enum nk_anti_aliasing AA )
{
	struct nk_vulkan_adapter *adapter = &ft.adapter;
	struct nk_buffer          vbuf, ebuf;

	// clang-format off
	struct Mat4f projection = {
		.m = { 2.0f, 0.0f, 0.0f, 0.0f,
			0.0f, -2.0f, 0.0f, 0.0f,
			0.0f, 0.0f, -1.0f, 0.0f,
			-1.0f, 1.0f, 0.0f, 1.0f },
	};
	// clang-format on

	projection.m[ 0 ] /= ( float ) ft.width;
	projection.m[ 5 ] /= ( float ) ft.height;

	void *data = ft_map_memory( adapter->device, adapter->uniform_buffer );
	memcpy( data, &projection, sizeof( projection ) );
	ft_unmap_memory( adapter->device, adapter->uniform_buffer );

	ft_cmd_set_viewport( cmd,
	                     0,
	                     0,
	                     ( float ) ft.display_width,
	                     ( float ) ft.display_height,
	                     0.0f,
	                     1.0f );
	ft_cmd_bind_pipeline( cmd, adapter->pipeline );
	ft_cmd_bind_descriptor_set( cmd, 0, adapter->set, adapter->pipeline );

	{
		/* convert from command queue into draw list and draw to screen */
		const struct nk_draw_command *draw_cmd;
		void		                 *vertices =
		    ft_map_memory( adapter->device, adapter->vertex_buffer );
		void *elements =
		    ft_map_memory( adapter->device, adapter->index_buffer );

		/* load draw vertices & elements directly into vertex + element buffer
		 */
		{
			/* fill convert configuration */
			struct nk_convert_config                          config;
			static const struct nk_draw_vertex_layout_element vertex_layout[] =
			    { { NK_VERTEX_POSITION,
			        NK_FORMAT_FLOAT,
			        NK_OFFSETOF( struct nk_ft_vertex, position ) },
			      { NK_VERTEX_TEXCOORD,
			        NK_FORMAT_FLOAT,
			        NK_OFFSETOF( struct nk_ft_vertex, uv ) },
			      { NK_VERTEX_COLOR,
			        NK_FORMAT_R8G8B8A8,
			        NK_OFFSETOF( struct nk_ft_vertex, col ) },
			      { NK_VERTEX_LAYOUT_END } };
			NK_MEMSET( &config, 0, sizeof( config ) );
			config.vertex_layout        = vertex_layout;
			config.vertex_size          = sizeof( struct nk_ft_vertex );
			config.vertex_alignment     = NK_ALIGNOF( struct nk_ft_vertex );
			config.null                 = adapter->null;
			config.circle_segment_count = 22;
			config.curve_segment_count  = 22;
			config.arc_segment_count    = 22;
			config.global_alpha         = 1.0f;
			config.shape_AA             = AA;
			config.line_AA              = AA;

			/* setup buffers to load vertices and elements */
			nk_buffer_init_fixed( &vbuf,
			                      vertices,
			                      ( size_t ) MAX_VERTEX_BUFFER );
			nk_buffer_init_fixed( &ebuf,
			                      elements,
			                      ( size_t ) MAX_INDEX_BUFFER );
			nk_convert( &ft.ctx, &adapter->cmds, &vbuf, &ebuf, &config );
		}
		ft_unmap_memory( adapter->device, adapter->vertex_buffer );
		ft_unmap_memory( adapter->device, adapter->index_buffer );

		/* iterate over and execute each draw command */
		ft_cmd_bind_vertex_buffer( cmd, adapter->vertex_buffer, 0 );
		ft_cmd_bind_index_buffer( cmd,
		                          adapter->index_buffer,
		                          0,
		                          FT_INDEX_TYPE_U16 );

		uint32_t index_offset = 0;
		nk_draw_foreach( draw_cmd, &ft.ctx, &adapter->cmds )
		{
			if ( !draw_cmd->elem_count )
				continue;

			ft_cmd_set_scissor(
			    cmd,
			    MAX( ( int32_t ) ( draw_cmd->clip_rect.x * ft.fb_scale.x ), 0 ),
			    MAX( ( int32_t ) ( draw_cmd->clip_rect.y * ft.fb_scale.y ), 0 ),
			    ( uint32_t ) ( draw_cmd->clip_rect.w * ft.fb_scale.x ),
			    ( uint32_t ) ( draw_cmd->clip_rect.h * ft.fb_scale.y ) );

			ft_cmd_draw_indexed( cmd,
			                     draw_cmd->elem_count,
			                     1,
			                     index_offset,
			                     0,
			                     0 );
			index_offset += draw_cmd->elem_count;
		}
		nk_clear( &ft.ctx );
	}
}

NK_API
void
nk_ft_shutdown( void )
{
	struct nk_vulkan_adapter *adapter = &ft.adapter;

	if ( adapter->font_tex )
	{
		ft_destroy_sampler( adapter->device, adapter->font_tex );
	}

	if ( adapter->font_image )
	{
		ft_destroy_image( adapter->device, adapter->font_image );
		nk_font_atlas_clear( &ft.atlas );
		nk_free( &ft.ctx );
	}

	ft_destroy_buffer( adapter->device, adapter->vertex_buffer );
	ft_destroy_buffer( adapter->device, adapter->index_buffer );
	ft_destroy_buffer( adapter->device, adapter->uniform_buffer );
	ft_destroy_pipeline( adapter->device, adapter->pipeline );
	ft_destroy_descriptor_set( adapter->device, adapter->set );
	ft_destroy_descriptor_set_layout( adapter->device, adapter->dsl );

	nk_ft_device_destroy();
	memset( &ft, 0, sizeof( ft ) );
}
