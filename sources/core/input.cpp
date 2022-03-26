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

	input_system->mouse_scroll = 0;
}

f32 get_mouse_pos_x( const InputSystem* input_system )
{
    return input_system->mouse_position[ 0 ];
}

f32 get_mouse_pos_y( const InputSystem* input_system )
{
    return input_system->mouse_position[ 1 ];
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

bool is_mouse_scrolled_up( const InputSystem* input_system )
{
	return input_system->mouse_scroll == 1;
}

bool is_mouse_scrolled_down( const InputSystem* input_system )
{
	return input_system->mouse_scroll == -1;
}

} // namespace fluent
