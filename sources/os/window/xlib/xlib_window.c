#include "xlib_window.h"

#if FT_PLATFORM_LINUX

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#define VK_USE_PLATFORM_XLIB_KHR
#include <volk/volk.h>
#include "log/log.h"
#include "os/key_codes.h"
#include "os/window/window.h"
#include "os/window/window_private.h"

struct ft_window
{
	Window   window;
	uint32_t width;
	uint32_t height;
};

struct
{
	Atom     delete_window_atom;
	Display* current_display;
	uint8_t  keyboard[ FT_KEY_COUNT + 1 ];
	uint32_t mouse;
	int32_t  mouse_position[ 2 ];
	XEvent   previous_event;
} xlib;

static const char* xlib_vulkan_extension_names[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
};

FT_INLINE enum ft_key_code
ft_keycode_from_xlib( KeySym keysym )
{
	switch ( keysym )
	{
	case XK_space: return FT_KEY_SPACE;
	case XK_apostrophe: return FT_KEY_APOSTROPHE;
	case XK_comma: return FT_KEY_COMMA;
	case XK_minus: return FT_KEY_MINUS;
	case XK_period: return FT_KEY_PERIOD;
	case XK_slash: return FT_KEY_SLASH;
	case XK_semicolon: return FT_KEY_SEMICOLON;
	case XK_equal: return FT_KEY_EQUAL;
	case XK_a:
	case XK_A: return FT_KEY_A;
	case XK_b:
	case XK_B: return FT_KEY_B;
	case XK_c:
	case XK_C: return FT_KEY_C;
	case XK_d:
	case XK_D: return FT_KEY_D;
	case XK_e:
	case XK_E: return FT_KEY_E;
	case XK_f:
	case XK_F: return FT_KEY_F;
	case XK_g:
	case XK_G: return FT_KEY_G;
	case XK_h:
	case XK_H: return FT_KEY_H;
	case XK_i:
	case XK_I: return FT_KEY_I;
	case XK_j:
	case XK_J: return FT_KEY_J;
	case XK_k:
	case XK_K: return FT_KEY_K;
	case XK_l:
	case XK_L: return FT_KEY_L;
	case XK_m:
	case XK_M: return FT_KEY_M;
	case XK_n:
	case XK_N: return FT_KEY_N;
	case XK_o:
	case XK_O: return FT_KEY_O;
	case XK_p:
	case XK_P: return FT_KEY_P;
	case XK_q:
	case XK_Q: return FT_KEY_Q;
	case XK_r:
	case XK_R: return FT_KEY_R;
	case XK_s:
	case XK_S: return FT_KEY_S;
	case XK_t:
	case XK_T: return FT_KEY_T;
	case XK_u:
	case XK_U: return FT_KEY_U;
	case XK_v:
	case XK_V: return FT_KEY_V;
	case XK_w:
	case XK_W: return FT_KEY_W;
	case XK_x:
	case XK_X: return FT_KEY_X;
	case XK_y:
	case XK_Y: return FT_KEY_Y;
	case XK_z:
	case XK_Z: return FT_KEY_Z;
	case XK_bracketleft: return FT_KEY_LEFT_BRACKET;
	case XK_backslash: return FT_KEY_BACKSLASH;
	case XK_bracketright: return FT_KEY_RIGHT_BRACKET;
	case XK_grave: return FT_KEY_GRAVE_ACCENT;
	case XK_Escape: return FT_KEY_ESCAPE;
	case XK_Return: return FT_KEY_ENTER;
	case XK_Tab: return FT_KEY_TAB;
	case XK_BackSpace: return FT_KEY_BACKSPACE;
	case XK_Insert: return FT_KEY_INSERT;
	case XK_Delete: return FT_KEY_DELETE;
	case XK_Right: return FT_KEY_RIGHT;
	case XK_Left: return FT_KEY_LEFT;
	case XK_Down: return FT_KEY_DOWN;
	case XK_Up: return FT_KEY_UP;
	case XK_Page_Up: return FT_KEY_PAGE_UP;
	case XK_Page_Down: return FT_KEY_PAGE_DOWN;
	case XK_Home: return FT_KEY_HOME;
	case XK_End: return FT_KEY_END;
	case XK_Caps_Lock: return FT_KEY_CAPS_LOCK;
	case XK_Scroll_Lock: return FT_KEY_SCROLL_LOCK;
	case XK_Num_Lock: return FT_KEY_NUM_LOCK;
	case XK_Print: return FT_KEY_PRINT_SCREEN;
	case XK_Pause: return FT_KEY_PAUSE;
	case XK_F1: return FT_KEY_F1;
	case XK_F2: return FT_KEY_F2;
	case XK_F3: return FT_KEY_F3;
	case XK_F4: return FT_KEY_F4;
	case XK_F5: return FT_KEY_F5;
	case XK_F6: return FT_KEY_F6;
	case XK_F7: return FT_KEY_F7;
	case XK_F8: return FT_KEY_F8;
	case XK_F9: return FT_KEY_F9;
	case XK_F10: return FT_KEY_F10;
	case XK_F11: return FT_KEY_F11;
	case XK_F12: return FT_KEY_F12;
	case XK_F13: return FT_KEY_F13;
	case XK_F14: return FT_KEY_F14;
	case XK_F15: return FT_KEY_F15;
	case XK_F16: return FT_KEY_F16;
	case XK_F17: return FT_KEY_F17;
	case XK_F18: return FT_KEY_F18;
	case XK_F19: return FT_KEY_F19;
	case XK_F20: return FT_KEY_F20;
	case XK_F21: return FT_KEY_F21;
	case XK_F22: return FT_KEY_F22;
	case XK_F23: return FT_KEY_F23;
	case XK_F24: return FT_KEY_F24;
	case XK_F25: return FT_KEY_F25;
	case XK_KP_0: return FT_KEY_KP0;
	case XK_KP_1: return FT_KEY_KP1;
	case XK_KP_2: return FT_KEY_KP2;
	case XK_KP_3: return FT_KEY_KP3;
	case XK_KP_4: return FT_KEY_KP4;
	case XK_KP_5: return FT_KEY_KP5;
	case XK_KP_6: return FT_KEY_KP6;
	case XK_KP_7: return FT_KEY_KP7;
	case XK_KP_8: return FT_KEY_KP8;
	case XK_KP_9: return FT_KEY_KP9;
	case XK_KP_Decimal: return FT_KEY_KP_DECIMAL;
	case XK_KP_Divide: return FT_KEY_KP_DIVIDE;
	case XK_KP_Multiply: return FT_KEY_KP_MULTIPLY;
	case XK_KP_Subtract: return FT_KEY_KP_SUBSTRACT;
	case XK_KP_Add: return FT_KEY_KP_ADD;
	case XK_KP_Enter: return FT_KEY_KP_ENTER;
	case XK_KP_Equal: return FT_KEY_KP_EQUAL;
	case XK_Shift_L: return FT_KEY_LEFT_SHIFT;
	case XK_Control_L: return FT_KEY_LEFT_CONTROL;
	case XK_Alt_L: return FT_KEY_LEFT_ALT;
	case XK_Super_L: return FT_KEY_LEFT_SUPER;
	case XK_Shift_R: return FT_KEY_RIGHT_SHIFT;
	case XK_Control_R: return FT_KEY_RIGHT_CONTROL;
	case XK_Alt_R: return FT_KEY_RIGHT_ALT;
	case XK_Super_R: return FT_KEY_RIGHT_SUPER;
	case XK_Menu: return FT_KEY_MENU;
	default: return FT_KEY_COUNT;
	}
};

FT_INLINE void
set_size_hints( Display* display,
                Window   window,
                uint32_t min_width,
                uint32_t min_height,
                uint32_t max_width,
                uint32_t max_height )
{
	XSizeHints hints;
	memset( &hints, 0, sizeof( hints ) );

	if ( min_width > 0 && min_height > 0 )
	{
		hints.flags |= PMinSize;
	}

	if ( max_width > 0 && max_height > 0 )
	{
		hints.flags |= PMaxSize;
	}

	hints.min_width  = min_width;
	hints.min_height = min_height;
	hints.max_width  = max_width;
	hints.max_height = max_height;

	XSetWMNormalHints( display, window, &hints );
}

void
xlib_destroy_window( struct ft_window* window )
{
	XDestroyWindow( xlib.current_display, window->window );
	XCloseDisplay( xlib.current_display );
	free( window );
}

static void
xlib_window_get_size( const struct ft_window* window,
                      uint32_t*               width,
                      uint32_t*               height )
{
	*width  = window->width;
	*height = window->height;
}

static void
xlib_window_get_framebuffer_size( const struct ft_window* window,
                                  uint32_t*               width,
                                  uint32_t*               height )
{
	*width  = window->width;
	*height = window->height;
}

static void
xlib_window_show_cursor( bool show )
{
}

static void
xlib_window_get_vulkan_instance_extensions( const struct ft_window* window,
                                            uint32_t*               count,
                                            const char**            names )
{
	if ( names == NULL )
	{
		*count = FT_ARRAY_SIZE( xlib_vulkan_extension_names );
	}
	else
	{
		for ( uint32_t i = 0; i < *count; ++i )
			names[ i ] = xlib_vulkan_extension_names[ i ];
	}
}

static void
xlib_window_create_vulkan_surface( const struct ft_window*      window,
                                   VkInstance                   instance,
                                   const VkAllocationCallbacks* allocator,
                                   VkSurfaceKHR*                surface )
{
	VkXlibSurfaceCreateInfoKHR info = {
	    .sType  = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
	    .window = window->window,
	    .dpy    = xlib.current_display,
	};

	VkResult result =
	    vkCreateXlibSurfaceKHR( instance, &info, allocator, surface );
	FT_ASSERT( result == VK_SUCCESS );
}

static int
xlib_window_poll_event( struct ft_event* event )
{
	int    r = 1;
	XEvent e;

	if ( XPending( xlib.current_display ) > 0 )
	{
		XNextEvent( xlib.current_display, &e );

		switch ( e.type )
		{
		case ClientMessage:
		{
			if ( e.xclient.data.l[ 0 ] == xlib.delete_window_atom )
			{
				event->type = FT_EVENT_TYPE_QUIT;
				break;
			}
		}
		case KeyPress:
		{
			bool is_repeat = xlib.previous_event.type == KeyRelease &&
			                 xlib.previous_event.xkey.time == e.xkey.time &&
			                 xlib.previous_event.xkey.keycode == e.xkey.keycode;

			enum ft_key_code key =
			    ft_keycode_from_xlib_keysym( XLookupKeysym( &e.xkey, 0 ) );
			xlib.keyboard[ key ] = 1 + ( uint8_t ) is_repeat;
			break;
		}
		case KeyRelease:
		{
			enum ft_key_code key =
			    ft_keycode_from_xlib_keysym( XLookupKeysym( &e.xkey, 0 ) );
			xlib.keyboard[ key ] = 0;
			break;
		}
		case MotionNotify:
		{
			xlib.mouse_position[ 0 ] = e.xmotion.x;
			xlib.mouse_position[ 1 ] = e.xmotion.y;
			break;
		}
		default:
		{
			r = 0;
			break;
		}
		}
	}
	else
	{
		r = 0;
	}

	return r;
}

static const uint8_t*
xlib_get_keyboard_state( uint32_t* key_count )
{
	*key_count = FT_KEY_COUNT;
	return xlib.keyboard;
}

static uint32_t
xlib_get_mouse_state( int32_t* x, int32_t* y )
{
	*x = xlib.mouse_position[ 0 ];
	*y = xlib.mouse_position[ 1 ];
	return 0;
}

struct ft_window*
xlib_create_window( const struct ft_window_info* info )
{
	ft_window_get_size_impl             = xlib_window_get_size;
	ft_window_get_framebuffer_size_impl = xlib_window_get_framebuffer_size;
	ft_destroy_window_impl              = xlib_destroy_window;
	ft_window_show_cursor_impl          = xlib_window_show_cursor;
	ft_window_poll_event_impl           = xlib_window_poll_event;
	ft_get_keyboard_state_impl          = xlib_get_keyboard_state;
	ft_get_mouse_state_impl             = xlib_get_mouse_state;
	ft_window_get_vulkan_instance_extensions_impl =
	    xlib_window_get_vulkan_instance_extensions;
	ft_window_create_vulkan_surface_impl = xlib_window_create_vulkan_surface;

	memset( &xlib, 0, sizeof( xlib ) );

	Display* display = XOpenDisplay( NULL );

	if ( display == NULL )
	{
		FT_WARN( "failed to open display" );
		return NULL;
	}

	int root   = DefaultRootWindow( display );
	int screen = DefaultScreen( display );

	int         screen_bit_depth = 24;
	XVisualInfo visual_info;
	memset( &visual_info, 0, sizeof( visual_info ) );

	if ( !XMatchVisualInfo( display,
	                        screen,
	                        screen_bit_depth,
	                        TrueColor,
	                        &visual_info ) )
	{
		FT_WARN( "no matching visual info" );
		return NULL;
	}

	XSetWindowAttributes attributes = {
	    .bit_gravity = StaticGravity, // no discard window content on resize
	    .background_pixel = 0,
	    .colormap =
	        XCreateColormap( display, root, visual_info.visual, AllocNone ),
	    .event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask |
	                  PointerMotionMask,
	};

	Window window =
	    XCreateWindow( display,
	                   root,
	                   0,
	                   0,
	                   info->width,
	                   info->height,
	                   0,
	                   visual_info.depth,
	                   InputOutput,
	                   visual_info.visual,
	                   CWBitGravity | CWBackPixel | CWColormap | CWEventMask,
	                   &attributes );

	if ( !window )
	{
		FT_WARN( "failed to create window" );
		return NULL;
	}

	XStoreName( display, window, info->title );

	XMapWindow( display, window );
	XFlush( display );

	Atom wm_delete_atom = XInternAtom( display, "WM_DELETE_WINDOW", False );

	if ( !XSetWMProtocols( display, window, &wm_delete_atom, 1 ) )
	{
		FT_WARN( "couldn't register WM_DELETE_WINDOW atom" );
	}

	if ( info->resizable )
	{
		set_size_hints( display, window, 200, 200, 0, 0 );
	}
	else
	{
		set_size_hints( display,
		                window,
		                info->width,
		                info->height,
		                info->width,
		                info->height );
	}

	xlib.current_display    = display;
	xlib.delete_window_atom = wm_delete_atom;

	struct ft_window* r = malloc( sizeof( struct ft_window ) );
	r->window           = window;
	r->width            = info->width;
	r->height           = info->height;

	return r;
}

#endif
