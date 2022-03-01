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
    f32 mouse_offset[ 2 ];
};

void init_input_system(InputSystem* input_system);
void update_input_system(InputSystem* input_system);
void update_input_keyboard_state(
    InputSystem* input_system, KeyCode key, u32 press);
void update_input_mouse_state(InputSystem* input_system, f32 x, f32 y);
void update_input_mouse_buttons_state(
    InputSystem* input_system, ButtonCode button, u32 press);

f32 get_mouse_offset_x(const InputSystem* input_system);
f32 get_mouse_offset_y(const InputSystem* input_system);

b32 is_key_pressed(const InputSystem* input_system, KeyCode key);
b32 is_button_pressed(const InputSystem* input_system, ButtonCode button);
} // namespace fluent
