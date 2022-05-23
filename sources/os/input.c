#include <SDL2/SDL.h>
#include "window.h"
#include "input.h"

i32
get_mouse_pos_x()
{
	i32 x, y;
	SDL_GetMouseState( &x, &y );
	return x;
}

i32
get_mouse_pos_y()
{
	i32 x, y;
	SDL_GetMouseState( &x, &y );
	return y;
}

void
get_mouse_position( i32* x, i32* y )
{
	SDL_GetMouseState( x, y );
}

b32
is_key_pressed( enum KeyCode key )
{
	i32       count;
	const u8* kb = SDL_GetKeyboardState( &count );
	FT_ASSERT( key < count );
	return kb[ key ];
}

b32
is_button_pressed( enum Button button )
{
	i32 x, y;
	u32 buttons = SDL_GetMouseState( &x, &y );
	return buttons & button;
}
