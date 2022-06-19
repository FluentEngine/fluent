#include <SDL.h>
#include "base/base.h"
#include "window.h"
#include "input.h"

FT_API int32_t
ft_get_mouse_pos_x()
{
	int32_t x, y;
	SDL_GetMouseState( &x, &y );
	return x;
}

FT_API int32_t
ft_get_mouse_pos_y()
{
	int32_t x, y;
	SDL_GetMouseState( &x, &y );
	return y;
}

FT_API void
ft_get_mouse_position( int32_t* x, int32_t* y )
{
	SDL_GetMouseState( x, y );
}

FT_API bool
ft_is_key_pressed( enum ft_key_code key )
{
	int32_t        count;
	const uint8_t* kb = SDL_GetKeyboardState( &count );
	FT_ASSERT( key < count );
	return kb[ key ];
}

FT_API bool
ft_is_button_pressed( enum ft_button button )
{
	int32_t  x, y;
	uint32_t buttons = SDL_GetMouseState( &x, &y );
	return buttons & button;
}
