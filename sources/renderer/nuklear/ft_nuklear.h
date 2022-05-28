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
#include "os/input.h"
#include "renderer/backend/renderer_backend.h"
#include "string.h"

NK_API struct nk_context *
nk_ft_init( struct WsiInfo *wsi_info,
            struct Device  *device,
            struct Queue   *queue,
            enum Format     color_format,
            enum Format     depth_format );

NK_API void
nk_ft_shutdown( void );
NK_API void
nk_ft_font_stash_begin( struct nk_font_atlas **atlas );
NK_API void
nk_ft_font_stash_end( void );
NK_API void
nk_ft_new_frame( void );

NK_API void
nk_ft_render( const struct CommandBuffer *cmd, enum nk_anti_aliasing AA );

NK_API void
nk_ft_device_destroy( void );
NK_API void
nk_ft_device_create( struct Device *,
                     struct Queue *graphics_queue,
                     enum Format,
                     enum Format );

NK_API void
nk_ft_char_callback( struct WsiInfo *, unsigned int codepoint );
NK_API void
nk_gflw3_scroll_callback( struct WsiInfo *, double xoff, double yoff );
NK_API void
nk_ft_mouse_button_callback( struct WsiInfo *,
                             int button,
                             int action,
                             int mods );
