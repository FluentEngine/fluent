#pragma once

#include "base/base.h"
#include "key_codes.h"
#include "mouse_codes.h"

#ifdef __cplusplus
extern "C"
{
#endif
	i32
	get_mouse_pos_x( void );

	i32
	get_mouse_pos_y( void );

	void
	get_mouse_position( i32* x, i32* y );

	b32
	is_key_pressed( enum KeyCode key );

	b32
	is_button_pressed( enum Button button );
#ifdef __cplusplus
}
#endif
