#pragma once

#include "base/base.h"

struct ft_device;
struct ft_render_pass;
struct ft_render_graph;
struct ft_depth_stencil_clear_value;

typedef bool ( *ft_get_clear_color_cb )( uint32_t, float ( * )[ 4 ] );
typedef bool ( *ft_get_clear_depth_stencil_cb )(
    struct ft_depth_stencil_clear_value* );
typedef void ( *ft_create_cb )( const struct ft_device*, void* );
typedef void ( *ft_execute_cb )( const struct ft_device* device,
                                 struct ft_command_buffer*,
                                 void* );
typedef void ( *ft_destroy_cb )( const struct ft_device*, void* );

FT_API void
ft_rg_create( struct ft_device* device, struct ft_render_graph** graph );

FT_API void
ft_rg_destroy( struct ft_render_graph* graph );

FT_API void
ft_rg_set_backbuffer_source( struct ft_render_graph* graph, const char* name );

FT_API void
ft_rg_build( struct ft_render_graph* graph );

FT_API void
ft_rg_set_swapchain_dimensions( struct ft_render_graph* graph,
                                uint32_t                width,
                                uint32_t                height );

FT_API void
ft_rg_setup_attachments( struct ft_render_graph* graph,
                         struct ft_image*        image );

FT_API void
ft_rg_execute( struct ft_command_buffer* cmd, struct ft_render_graph* graph );

FT_API void
ft_rg_add_pass( struct ft_render_graph* graph,
                const char*             name,
                struct ft_render_pass** pass );

FT_API void
ft_rg_add_color_output( struct ft_render_pass*      pass,
                        const char*                 name,
                        const struct ft_image_info* info );

FT_API void
ft_rg_add_depth_stencil_output( struct ft_render_pass*      pass,
                                const char*                 name,
                                const struct ft_image_info* info );

FT_API void
ft_rg_set_user_data( struct ft_render_pass* pass, void* data );

FT_API void
ft_rg_set_pass_create_callback( struct ft_render_pass* pass, ft_create_cb cb );

FT_API void
ft_rg_set_pass_execute_callback( struct ft_render_pass* pass,
                                 ft_execute_cb          cb );

FT_API void
ft_rg_set_pass_destroy_callback( struct ft_render_pass* pass,
                                 ft_destroy_cb          cb );

FT_API void
ft_rg_set_get_clear_color( struct ft_render_pass* pass,
                           ft_get_clear_color_cb  cb );

FT_API void
ft_rg_set_get_clear_depth_stencil( struct ft_render_pass*        pass,
                                   ft_get_clear_depth_stencil_cb cb );
