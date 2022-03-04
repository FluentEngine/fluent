#include <SDL.h>
#include "core/window.hpp"
#include "core/input.hpp"

namespace fluent
{

void init_input_system( InputSystem* input_system )
{
    std::memset( input_system->keys,
                 0,
                 static_cast<size_t>( Key::Last ) *
                     sizeof( input_system->keys[ 0 ] ) );
    std::memset( input_system->buttons,
                 0,
                 static_cast<size_t>( Button::Last ) *
                     sizeof( input_system->buttons[ 0 ] ) );

    std::memset( input_system->mouse_offset,
                 0,
                 2 * sizeof( input_system->mouse_offset[ 0 ] ) );

    i32 x, y;
    SDL_GetMouseState( &x, &y );
}

void update_input_system( InputSystem* input_system )
{
    std::memset( input_system->buttons,
                 0,
                 static_cast<size_t>( Button::Last ) *
                     sizeof( input_system->buttons[ 0 ] ) );
}

void update_input_keyboard_state( InputSystem* input_system,
                                  KeyCode key,
                                  u32 press )
{
    input_system->keys[ key ] = press;
}

void update_input_mouse_state( InputSystem* input_system, f32 x, f32 y )
{
    input_system->mouse_offset[ 0 ] = x;
    input_system->mouse_offset[ 1 ] = y;
}

void update_input_mouse_buttons_state( InputSystem* input_system,
                                       ButtonCode button,
                                       u32 press )
{
    input_system->buttons[ button ] = press;
}

f32 get_mouse_offset_x( const InputSystem* input_system )
{
    return input_system->mouse_offset[ 0 ];
}

f32 get_mouse_offset_y( const InputSystem* input_system )
{
    return input_system->mouse_offset[ 1 ];
}

b32 is_key_pressed( const InputSystem* input_system, KeyCode key )
{
    return input_system->keys[ key ];
}

b32 is_button_pressed( const InputSystem* input_system, ButtonCode button )
{
    return input_system->buttons[ button ];
}
} // namespace fluent
