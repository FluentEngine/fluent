#pragma once

#include "core/base.hpp"
#include "core/key_codes.hpp"
#include "core/mouse_codes.hpp"

namespace fluent
{

struct Window;

struct InputSystem
{
    u32 keys[ Key::Last ];
    u32 buttons[ Button::Last ];
    f32 mouse_position[ 2 ];
    f32 mouse_offset[ 2 ];
	i32 mouse_scroll = 0;
};

void init_input_system( InputSystem* input_system );
void update_input_system( InputSystem* input_system );

f32 get_mouse_pos_x( const InputSystem* input_system );
f32 get_mouse_pos_y( const InputSystem* input_system );
f32 get_mouse_offset_x( const InputSystem* input_system );
f32 get_mouse_offset_y( const InputSystem* input_system );

b32  is_key_pressed( const InputSystem* input_system, KeyCode key );
b32  is_button_pressed( const InputSystem* input_system, ButtonCode button );
bool is_mouse_scrolled_up( const InputSystem* input_system );
bool is_mouse_scrolled_down( const InputSystem* input_system );

} // namespace fluent
