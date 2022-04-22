#pragma once

#include "core/base.hpp"
#include "core/key_codes.hpp"
#include "core/mouse_codes.hpp"

namespace fluent
{

struct Window;

struct InputSystem
{
	Window* window;
	u32     keys[ Key::Last ];
	u32     buttons[ Button::Last ];
	f32     mouse_position[ 2 ];
	f32     mouse_offset[ 2 ];
	i32     mouse_scroll = 0;
};

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
is_button_pressed( ButtonCode button );
bool
is_mouse_scrolled_up();
bool
is_mouse_scrolled_down();

} // namespace fluent
