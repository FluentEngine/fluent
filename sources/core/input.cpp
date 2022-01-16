#include <SDL.h>
#include "core/window.hpp"
#include "core/input.hpp"

namespace fluent
{

void init_input_system(InputSystem* input_system)
{
    std::memset(input_system->keys, 0, KEYS_COUNT * sizeof(input_system->keys[ 0 ]));

    i32 x, y;
    SDL_GetMouseState(&x, &y);

    input_system->mouse_position[ 0 ]     = x;
    input_system->mouse_position[ 1 ]     = y;
    input_system->old_mouse_position[ 0 ] = x;
    input_system->old_mouse_position[ 1 ] = y;
}

void update_input_system(InputSystem* input_system)
{
    input_system->old_mouse_position[ 0 ] = input_system->mouse_position[ 0 ];
    input_system->old_mouse_position[ 1 ] = input_system->mouse_position[ 1 ];
}

void update_input_keyboard_state(InputSystem* input_system, KeyCode key, u32 press)
{
    input_system->keys[ key ] = press;
}

void update_input_mouse_state(InputSystem* input_system, f32 x, f32 y)
{
    input_system->old_mouse_position[ 0 ] = input_system->mouse_position[ 0 ];
    input_system->old_mouse_position[ 1 ] = input_system->mouse_position[ 1 ];
    input_system->mouse_position[ 0 ]     = x;
    input_system->mouse_position[ 1 ]     = y;
}

f32 get_mouse_pos_x(const InputSystem* input_system)
{
    return input_system->mouse_position[ 0 ];
}

f32 get_mouse_pos_y(const InputSystem* input_system)
{
    return input_system->mouse_position[ 1 ];
}

f32 get_mouse_offset_x(const InputSystem* input_system)
{
    return input_system->mouse_position[ 0 ] - input_system->old_mouse_position[ 0 ];
}

f32 get_mouse_offset_y(const InputSystem* input_system)
{
    return input_system->old_mouse_position[ 1 ] - input_system->mouse_position[ 1 ];
}

b32 is_key_pressed(const InputSystem* input_system, KeyCode key)
{
    return input_system->keys[ key ];
}

b32 is_key_released(const InputSystem* input_system, KeyCode key)
{
    return !input_system->keys[ key ];
}

} // namespace fluent
