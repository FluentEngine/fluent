#include <SDL.h>
#include "core/window.hpp"
#include "core/input.hpp"

namespace fluent
{

static InputSystem input_system;

InputSystem*
get_input_system()
{
    return &input_system;
}

void
init_input_system( Window* window )
{
    input_system.window = window;

    std::memset( input_system.keys,
                 0,
                 static_cast<size_t>( Key::Last ) *
                     sizeof( input_system.keys[ 0 ] ) );

    std::memset( input_system.buttons,
                 0,
                 static_cast<size_t>( Button::Last ) *
                     sizeof( input_system.buttons[ 0 ] ) );

    std::memset( input_system.mouse_offset,
                 0,
                 2 * sizeof( input_system.mouse_offset[ 0 ] ) );

    i32 x, y;
    SDL_GetMouseState( &x, &y );
}

void
update_input_system()
{
    std::memset( input_system.buttons,
                 0,
                 static_cast<size_t>( Button::Last ) *
                     sizeof( input_system.buttons[ 0 ] ) );

    input_system.mouse_scroll = 0;

    i32 x, y;
    SDL_GetRelativeMouseState( &x, &y );
    input_system.mouse_offset[ 0 ] = x;
    input_system.mouse_offset[ 1 ] = y;
    SDL_GetGlobalMouseState( &x, &y );
    input_system.mouse_position[ 0 ] = x;
    input_system.mouse_position[ 1 ] = y;
}

f32
get_mouse_pos_x()
{
    return input_system.mouse_position[ 0 ];
}

f32
get_mouse_pos_y()
{
    return input_system.mouse_position[ 1 ];
}

f32
get_mouse_offset_x()
{
    return input_system.mouse_offset[ 0 ];
}

f32
get_mouse_offset_y()
{
    return input_system.mouse_offset[ 1 ];
}

b32
is_key_pressed( KeyCode key )
{
    return input_system.keys[ key ];
}

b32
is_button_pressed( ButtonCode button )
{
    return input_system.buttons[ button ];
}

bool
is_mouse_scrolled_up()
{
    return input_system.mouse_scroll == 1;
}

bool
is_mouse_scrolled_down()
{
    return input_system.mouse_scroll == -1;
}

} // namespace fluent
