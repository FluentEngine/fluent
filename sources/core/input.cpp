#include <SDL.h>
#include "core/window.hpp"
#include "core/input.hpp"

namespace fluent
{

void init_input_system(InputSystem* input_system)
{
    std::memset(input_system->keys, 0, KEYS_COUNT * sizeof(input_system->keys[ 0 ]));
    std::memset(input_system->mouse_offsets, 0, 2 * sizeof(input_system->mouse_offsets[ 0 ]));
    SDL_GetMouseState(&input_system->mouse_position[ 0 ], &input_system->mouse_position[ 1 ]);
}

void update_input_keyboard_state(InputSystem* input_system, KeyCode key, u32 press)
{
    input_system->keys[ key ] = press;
}

void update_input_mouse_state(InputSystem* input_system, i32 x, i32 y)
{
    input_system->mouse_offsets[ 0 ] = input_system->mouse_position[ 0 ] - x;
    input_system->mouse_offsets[ 1 ] = input_system->mouse_position[ 1 ] - y;
    input_system->mouse_position[ 0 ] = x;
    input_system->mouse_position[ 1 ] = y;
}

i32 get_mouse_pos_x(const InputSystem* input_system)
{
    return input_system->mouse_position[ 0 ];
}

i32 get_mouse_pos_y(const InputSystem* input_system)
{
    return input_system->mouse_position[ 1 ];
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
