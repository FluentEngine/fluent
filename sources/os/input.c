#include <SDL.h>
#include "base/base.h"
#include "log/log.h"
#include "window.h"
#include "input.h"

struct ft_input_state
{
	int32_t wheel;
};

static struct ft_input_state input_state;

void
ft_input_update( int32_t wheel )
{
	input_state.wheel = wheel;
}

int32_t
ft_get_mouse_pos_x()
{
	int32_t x, y;
	ft_get_mouse_position( &x, &y );
	return x;
}

int32_t
ft_get_mouse_pos_y()
{
	int32_t x, y;
	ft_get_mouse_position( &x, &y );
	return y;
}

void
ft_get_mouse_position( int32_t* x, int32_t* y )
{
	SDL_GetGlobalMouseState( x, y );
}

int32_t
ft_get_mouse_offset_x()
{
	int32_t x, y;
	ft_get_mouse_offset( &x, &y );
	return x;
}

int32_t
ft_get_mouse_offset_y()
{
	int32_t x, y;
	ft_get_mouse_offset( &x, &y );
	return y;
}

void
ft_get_mouse_offset( int32_t* x, int32_t* y )
{
	SDL_GetRelativeMouseState( x, y );
}

bool
ft_is_key_pressed( enum ft_key_code key )
{
	int32_t        count;
	const uint8_t* kb = SDL_GetKeyboardState( &count );
	FT_ASSERT( key < count );
	return kb[ key ];
}

bool
ft_is_button_pressed( enum ft_button button )
{
	int32_t  x, y;
	uint32_t buttons = SDL_GetMouseState( &x, &y );
	return ( buttons & ( 1 << ( button - 1 ) ) );
}

int32_t
ft_get_mouse_wheel( void )
{
	return input_state.wheel;
}
