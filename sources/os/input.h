#pragma once

#include "base/base.h"
#include "key_codes.h"
#include "mouse_codes.h"

#ifdef __cplusplus
extern "C"
{
#endif
	struct Window;

	struct InputSystem
	{
		struct Window* window;
		u32            keys[ FT_KEY_COUNT ];
		u32            buttons[ FT_BUTTON_COUNT ];
		f32            mouse_position[ 2 ];
		f32            mouse_offset[ 2 ];
		i32            mouse_scroll;
	};

	void
	init_input_system( struct Window* window );

	void
	update_input_system( void );

	f32
	get_mouse_pos_x( void );

	f32
	get_mouse_pos_y( void );

	f32
	get_mouse_offset_x( void );

	f32
	get_mouse_offset_y( void );

	b32
	is_key_pressed( enum KeyCode key );

	b32
	is_button_pressed( enum Button button );

	b32
	is_mouse_scrolled_up( void );

	b32
	is_mouse_scrolled_down( void );

#ifdef __cplusplus
}
#endif
