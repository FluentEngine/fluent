#pragma once

#include "base/base.h"

struct RenderPass;
struct RenderGraph;

typedef b32 ( *get_clear_color_cb )( u32, f32* [4] );
typedef b32 ( *get_clear_depth_stencil_cb )( f32*, u32* );
typedef void ( *create_cb )( void* );
typedef void ( *execute_cb )( void* );
typedef void ( *destroy_cb )( void* );

void
rg_create( struct Device* device, struct RenderGraph** graph );

void
rg_destroy( struct RenderGraph* graph );

void
rg_set_backbuffer_source( struct RenderGraph* graph, const char* name );

void
rg_build( struct RenderGraph* graph );

void
rg_setup_attachments( struct RenderGraph* graph, struct Image* image );

void
rg_execute( struct CommandBuffer* cmd, struct RenderGraph* graph );

void
rg_add_pass( struct RenderGraph* graph,
             const char*         name,
             struct RenderPass** pass );

void
rg_add_color_output( struct RenderPass*      pass,
                     const char*             name,
                     const struct ImageInfo* info );

void
rg_set_user_data( struct RenderPass* pass, void* data );

void
rg_set_pass_create_callback( struct RenderPass* pass, create_cb cb );

void
rg_set_pass_execute_callback( struct RenderPass* pass, execute_cb cb );

void
rg_set_pass_destroy_callback( struct RenderPass* pass, destroy_cb cb );

void
rg_set_get_clear_color( struct RenderPass* pass, get_clear_color_cb cb );

void
rg_set_get_clear_depth_stencil( struct RenderPass*         pass,
                                get_clear_depth_stencil_cb cb );
