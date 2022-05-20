#pragma once

#include "base/base.h"
#include "key_codes.h"
#include "mouse_codes.h"

#ifdef __cplusplus
extern "C"
{
#endif
	struct Window;

	typedef struct InputSystem
	{
		Window* window;
		u32     keys[ FT_KEY_COUNT ];
		u32     buttons[ FT_BUTTON_COUNT ];
		f32     mouse_position[ 2 ];
		f32     mouse_offset[ 2 ];
		i32     mouse_scroll;
	} InputSystem;

	void
	init_input_system( Window* window );

	void
	update_input_system();

	f32
	get_mouse_pos_x();

	f32
	get_mouse_pos_y();

	f32
	get_mouse_offset_x();

	f32
	get_mouse_offset_y();

	b32
	is_key_pressed( KeyCode key );

	b32
	is_button_pressed( Button button );

	b32
	is_mouse_scrolled_up();

	b32
	is_mouse_scrolled_down();

#ifdef __cplusplus
}
#endif
