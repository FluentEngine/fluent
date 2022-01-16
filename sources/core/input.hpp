#pragma once

#include "core/base.hpp"
#include "core/key_codes.hpp"

namespace fluent
{

struct Window;

static constexpr u32 KEYS_COUNT = 256;

struct InputSystem
{
    u32 keys[ KEYS_COUNT ];
    f32 old_mouse_position[ 2 ];
    f32 mouse_position[ 2 ];
};

void init_input_system(InputSystem* input_system);
void update_input_system(InputSystem* input_system);
void update_input_keyboard_state(InputSystem* input_system, KeyCode key, u32 press);
void update_input_mouse_state(InputSystem* input_system, f32 x, f32 y);

f32 get_mouse_pos_x(const InputSystem* input_system);
f32 get_mouse_pos_y(const InputSystem* input_system);
f32 get_mouse_offset_x(const InputSystem* input_system);
f32 get_mouse_offset_y(const InputSystem* input_system);

b32 is_key_pressed(const InputSystem* input_system, KeyCode key);
b32 is_key_released(const InputSystem* input_system, KeyCode key);

} // namespace fluent
