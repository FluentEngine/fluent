#include <SDL.h>
#include "window.h"
#include "input.h"

static struct InputSystem input_system;

struct InputSystem*
get_input_system( void );

struct InputSystem*
get_input_system()
{
	return &input_system;
}

void
init_input_system( struct Window* window )
{
	input_system.window       = window;
	input_system.mouse_scroll = 0;

	memset( input_system.keys,
	        0,
	        ( u32 ) ( FT_KEY_COUNT ) * sizeof( input_system.keys[ 0 ] ) );

	memset( input_system.buttons,
	        0,
	        ( u32 ) ( FT_BUTTON_COUNT ) * sizeof( input_system.buttons[ 0 ] ) );

	memset( input_system.mouse_offset,
	        0,
	        2 * sizeof( input_system.mouse_offset[ 0 ] ) );

	i32 x, y;
	SDL_GetMouseState( &x, &y );
}

void
update_input_system()
{
	memset( input_system.buttons,
	        0,
	        ( u32 ) ( FT_BUTTON_COUNT ) * sizeof( input_system.buttons[ 0 ] ) );

	input_system.mouse_scroll = 0;

	i32 x, y;
	SDL_GetRelativeMouseState( &x, &y );
	input_system.mouse_offset[ 0 ] = ( f32 ) x;
	input_system.mouse_offset[ 1 ] = ( f32 ) y;
	SDL_GetGlobalMouseState( &x, &y );
	input_system.mouse_position[ 0 ] = ( f32 ) x;
	input_system.mouse_position[ 1 ] = ( f32 ) y;
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
is_key_pressed( enum KeyCode key )
{
	return input_system.keys[ key ];
}

b32
is_button_pressed( enum Button button )
{
	return input_system.buttons[ button ];
}

b32
is_mouse_scrolled_up()
{
	return input_system.mouse_scroll == 1;
}

b32
is_mouse_scrolled_down()
{
	return input_system.mouse_scroll == -1;
}
