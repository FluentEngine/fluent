#include "base/base.h"
#include "window/window.h"
#include "input.h"

struct ft_input_state
{
	int32_t old_mouse_position[ 2 ];
};

static struct ft_input_state input_state;

void
ft_input_update()
{
	ft_get_mouse_state( &input_state.old_mouse_position[ 0 ],
	                    &input_state.old_mouse_position[ 1 ] );
}

int32_t
ft_get_mouse_pos_x()
{
	int32_t x, y;
	ft_get_mouse_state( &x, &y );
	return x;
}

int32_t
ft_get_mouse_pos_y()
{
	int32_t x, y;
	ft_get_mouse_state( &x, &y );
	return y;
}

void
ft_get_mouse_offset( int32_t* x, int32_t* y )
{
	ft_get_mouse_state( x, y );
	*x = ( *x - input_state.old_mouse_position[ 0 ] );
	*y = ( *y - input_state.old_mouse_position[ 1 ] );
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

bool
ft_is_key_pressed( enum ft_key_code key )
{
	const uint8_t* kb = ft_get_keyboard_state();
	return kb[ key ] > 0;
}

bool
ft_is_button_pressed( enum ft_button button )
{
	int32_t        x, y;
	const uint8_t* buttons = ft_get_mouse_state( &x, &y );
	return buttons[ button ];
}

int32_t
ft_get_mouse_wheel( void )
{
	// TODO:
	return 0;
}
