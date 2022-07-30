#pragma once

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include <nuklear/nuklear.h>
#include "wsi/wsi.h"
#include "window/input.h"
#include "renderer/backend/renderer_backend.h"
#include "string.h"

#ifdef __cplusplus
extern "C"
{
#endif
	NK_API struct nk_context *
	nk_ft_init( struct ft_wsi_info *wsi_info,
	            struct ft_device   *device,
	            struct ft_queue    *queue,
	            enum ft_format      color_format,
	            enum ft_format      depth_format );

	FT_API void
	nk_ft_shutdown( void );
	NK_API void
	nk_ft_font_stash_begin( struct nk_font_atlas **atlas );
	NK_API void
	nk_ft_font_stash_end( void );
	NK_API void
	nk_ft_new_frame( void );

	NK_API void
	nk_ft_render( const struct ft_command_buffer *cmd,
	              enum nk_anti_aliasing           AA );

	NK_API void
	nk_ft_device_destroy( void );
	NK_API void
	nk_ft_device_create( struct ft_device *,
	                     struct ft_queue *graphics_queue,
	                     enum ft_format,
	                     enum ft_format );

	NK_API struct nk_context *
	nk_ft_init( struct ft_wsi_info *wsi_info,
	            struct ft_device   *device,
	            struct ft_queue    *queue,
	            enum ft_format      color_format,
	            enum ft_format      depth_format );

	FT_API void
	nk_ft_shutdown( void );
	NK_API void
	nk_ft_font_stash_begin( struct nk_font_atlas **atlas );
	NK_API void
	nk_ft_font_stash_end( void );
	NK_API void
	nk_ft_new_frame( void );

	NK_API void
	nk_ft_render( const struct ft_command_buffer *cmd,
	              enum nk_anti_aliasing           AA );

	NK_API void
	nk_ft_device_destroy( void );
	NK_API void
	nk_ft_device_create( struct ft_device *,
	                     struct ft_queue *graphics_queue,
	                     enum ft_format,
	                     enum ft_format );

	NK_API void
	nk_ft_char_callback( struct ft_wsi_info *, unsigned int codepoint );
	NK_API void
	nk_gflw3_scroll_callback( struct ft_wsi_info *, double xoff, double yoff );
	NK_API void
	nk_ft_mouse_button_callback( struct ft_wsi_info *,
	                             int button,
	                             int action,
	                             int mods );

#ifdef __cplusplus
}
#endif
