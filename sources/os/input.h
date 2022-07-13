#pragma once

#include "base/base.h"
#include "os/window/window.h"
#include "key_codes.h"
#include "mouse_codes.h"

FT_API int32_t
ft_get_mouse_pos_x( void );

FT_API int32_t
ft_get_mouse_pos_y( void );

FT_API void
ft_get_mouse_position( int32_t* x, int32_t* y );

FT_API int32_t
ft_get_mouse_offset_x( void );

FT_API int32_t
ft_get_mouse_offset_y( void );

FT_API bool
ft_is_key_pressed( enum ft_key_code key );

FT_API bool
ft_is_button_pressed( enum ft_button button );

FT_API int32_t
ft_get_mouse_wheel( void );
