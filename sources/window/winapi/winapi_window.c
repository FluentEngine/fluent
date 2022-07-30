#include "winapi_window.h"

#if FT_PLATFORM_WINDOWS

#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <volk/volk.h>
#include "log/log.h"
#include "os/key_codes.h"
#include "os/window/window.h"
#include "os/window/window_private.h"

#define FT_WINDOW_CLASS L"Fluent"

struct ft_window
{
	HWND     window;
	uint32_t width;
	uint32_t height;
	bool     should_close;
};

struct
{
	uint8_t keyboard[ FT_KEY_COUNT + 1 ];
	int32_t mouse_position[ 2 ];
} winapi;

static const char* winapi_vulkan_extension_names[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
};

FT_INLINE enum ft_key_code
ft_keycode_from_winapi( int32_t scancode )
{
	switch ( scancode )
	{
	case 0x039: return FT_KEY_SPACE;
	case 0x028: return FT_KEY_APOSTROPHE;
	case 0x033: return FT_KEY_COMMA;
	case 0x00c: return FT_KEY_MINUS;
	case 0x034: return FT_KEY_PERIOD;
	case 0x035: return FT_KEY_SLASH;
	case 0x027: return FT_KEY_SEMICOLON;
	case 0x00d: return FT_KEY_EQUAL;
	case 0x01e: return FT_KEY_A;
	case 0x030: return FT_KEY_B;
	case 0x02e: return FT_KEY_C;
	case 0x020: return FT_KEY_D;
	case 0x012: return FT_KEY_E;
	case 0x021: return FT_KEY_F;
	case 0x022: return FT_KEY_G;
	case 0x023: return FT_KEY_H;
	case 0x017: return FT_KEY_I;
	case 0x024: return FT_KEY_J;
	case 0x025: return FT_KEY_K;
	case 0x026: return FT_KEY_L;
	case 0x032: return FT_KEY_M;
	case 0x031: return FT_KEY_N;
	case 0x018: return FT_KEY_O;
	case 0x019: return FT_KEY_P;
	case 0x010: return FT_KEY_Q;
	case 0x013: return FT_KEY_R;
	case 0x01f: return FT_KEY_S;
	case 0x014: return FT_KEY_T;
	case 0x016: return FT_KEY_U;
	case 0x02f: return FT_KEY_V;
	case 0x011: return FT_KEY_W;
	case 0x02d: return FT_KEY_X;
	case 0x015: return FT_KEY_Y;
	case 0x02c: return FT_KEY_Z;
	case 0x01a: return FT_KEY_LEFT_BRACKET;
	case 0x02b: return FT_KEY_BACKSLASH;
	case 0x01b: return FT_KEY_RIGHT_BRACKET;
	case 0x029: return FT_KEY_GRAVE_ACCENT;
	case 0x001: return FT_KEY_ESCAPE;
	case 0x01c: return FT_KEY_ENTER;
	case 0x00f: return FT_KEY_TAB;
	case 0x00e: return FT_KEY_BACKSPACE;
	case 0x152: return FT_KEY_INSERT;
	case 0x153: return FT_KEY_DELETE;
	case 0x14d: return FT_KEY_RIGHT;
	case 0x14b: return FT_KEY_LEFT;
	case 0x150: return FT_KEY_DOWN;
	case 0x148: return FT_KEY_UP;
	case 0x149: return FT_KEY_PAGE_UP;
	case 0x151: return FT_KEY_PAGE_DOWN;
	case 0x147: return FT_KEY_HOME;
	case 0x14f: return FT_KEY_END;
	case 0x03a: return FT_KEY_CAPS_LOCK;
	case 0x046: return FT_KEY_SCROLL_LOCK;
	case 0x145: return FT_KEY_NUM_LOCK;
	case 0x137: return FT_KEY_PRINT_SCREEN;
	case 0x045: return FT_KEY_PAUSE;
	case 0x03b: return FT_KEY_F1;
	case 0x03c: return FT_KEY_F2;
	case 0x03d: return FT_KEY_F3;
	case 0x03e: return FT_KEY_F4;
	case 0x03f: return FT_KEY_F5;
	case 0x040: return FT_KEY_F6;
	case 0x041: return FT_KEY_F7;
	case 0x042: return FT_KEY_F8;
	case 0x043: return FT_KEY_F9;
	case 0x044: return FT_KEY_F10;
	case 0x057: return FT_KEY_F11;
	case 0x058: return FT_KEY_F12;
	case 0x064: return FT_KEY_F13;
	case 0x065: return FT_KEY_F14;
	case 0x066: return FT_KEY_F15;
	case 0x067: return FT_KEY_F16;
	case 0x068: return FT_KEY_F17;
	case 0x069: return FT_KEY_F18;
	case 0x06a: return FT_KEY_F19;
	case 0x06b: return FT_KEY_F20;
	case 0x06c: return FT_KEY_F21;
	case 0x06d: return FT_KEY_F22;
	case 0x06e: return FT_KEY_F23;
	case 0x076: return FT_KEY_F24;
	case 0x077: return FT_KEY_F25;
	case 0x052: return FT_KEY_KP0;
	case 0x04f: return FT_KEY_KP1;
	case 0x050: return FT_KEY_KP2;
	case 0x051: return FT_KEY_KP3;
	case 0x04b: return FT_KEY_KP4;
	case 0x04c: return FT_KEY_KP5;
	case 0x04d: return FT_KEY_KP6;
	case 0x047: return FT_KEY_KP7;
	case 0x048: return FT_KEY_KP8;
	case 0x049: return FT_KEY_KP9;
	case 0x053: return FT_KEY_KP_DECIMAL;
	case 0x135: return FT_KEY_KP_DIVIDE;
	case 0x037: return FT_KEY_KP_MULTIPLY;
	case 0x04a: return FT_KEY_KP_SUBSTRACT;
	case 0x04e: return FT_KEY_KP_ADD;
	case 0x11c: return FT_KEY_KP_ENTER;
	case 0x059: return FT_KEY_KP_EQUAL;
	case 0x02a: return FT_KEY_LEFT_SHIFT;
	case 0x01d: return FT_KEY_LEFT_CONTROL;
	case 0x038: return FT_KEY_LEFT_ALT;
	case 0x15b: return FT_KEY_LEFT_SUPER;
	case 0x036: return FT_KEY_RIGHT_SHIFT;
	case 0x11d: return FT_KEY_RIGHT_CONTROL;
	case 0x138: return FT_KEY_RIGHT_ALT;
	case 0x15c: return FT_KEY_RIGHT_SUPER;
	case 0x15d: return FT_KEY_MENU;
	default: return FT_KEY_COUNT;
	}
};

FT_INLINE int32_t
extract_scancode( LPARAM l_param )
{
	return ( HIWORD( l_param ) & ( KF_EXTENDED | 0xff ) );
}

FT_INLINE int32_t
extract_mouse_pos_x( LPARAM l_param )
{
	return ( ( int32_t ) ( short ) LOWORD( l_param ) );
}

FT_INLINE int32_t
extract_mouse_pos_y( LPARAM l_param )
{
	return ( ( int32_t ) ( short ) HIWORD( l_param ) );
}

static LRESULT CALLBACK
wnd_proc( HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param )
{
	switch ( msg )
	{
	case WM_CREATE:
	{
		CREATESTRUCT* create = ( CREATESTRUCT* ) l_param;
		SetWindowLongPtr( hwnd,
		                  GWLP_USERDATA,
		                  ( LONG_PTR ) create->lpCreateParams );
		break;
	}
	case WM_MOUSEMOVE:
	{
		winapi.mouse_position[ 0 ] = extract_mouse_pos_x( l_param );
		winapi.mouse_position[ 1 ] = extract_mouse_pos_y( l_param );
		break;
	}
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		int32_t scancode = extract_scancode( l_param );
		winapi.keyboard[ ft_keycode_from_winapi( scancode ) ] = 1;
		break;
	}
	case WM_SYSKEYUP:
	case WM_KEYUP:
	{
		int32_t scancode = extract_scancode( l_param );
		winapi.keyboard[ ft_keycode_from_winapi( scancode ) ] = 0;
		break;
	}
	case WM_CLOSE:
	{
		struct ft_window* window = GetWindowLongPtr( hwnd, GWLP_USERDATA );
		window->should_close     = true;
		break;
	}
	case WM_DESTROY:
	{
		PostQuitMessage( 0 );
	}
	default: return DefWindowProc( hwnd, msg, w_param, l_param ); break;
	}
	return 0;
}

static void
winapi_destroy_window( struct ft_window* window )
{
	DestroyWindow( window->window );
	free( window );
}

static void
winapi_window_get_size( const struct ft_window* window,
                        uint32_t*               width,
                        uint32_t*               height )
{
	*width  = window->width;
	*height = window->height;
}

static void
winapi_window_get_framebuffer_size( const struct ft_window* window,
                                    uint32_t*               width,
                                    uint32_t*               height )
{
	winapi_window_get_size( window, width, height );
}

static void
winapi_window_show_cursor( bool show )
{
}

static bool
winapi_window_should_close( const struct ft_window* window )
{
	return window->should_close;
}

static void
winapi_window_get_vulkan_instance_extensions( const struct ft_window* window,
                                              uint32_t*               count,
                                              const char**            names )
{
	if ( names == NULL )
	{
		*count = FT_ARRAY_SIZE( winapi_vulkan_extension_names );
	}
	else
	{
		for ( uint32_t i = 0; i < *count; ++i )
			names[ i ] = winapi_vulkan_extension_names[ i ];
	}
}

static void
winapi_window_create_vulkan_surface( const struct ft_window*      window,
                                     VkInstance                   instance,
                                     const VkAllocationCallbacks* allocator,
                                     VkSurfaceKHR*                surface )
{
	VkWin32SurfaceCreateInfoKHR info = {
	    .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
	    .hinstance = GetModuleHandle( NULL ),
	    .hwnd      = window->window,
	};

	vkCreateWin32SurfaceKHR( instance, &info, allocator, surface );
}

static void
winapi_poll_events()
{
	MSG msg;
	while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
}

static const uint8_t*
winapi_get_keyboard_state( uint32_t* key_count )
{
	*key_count = FT_KEY_COUNT;
	return winapi.keyboard;
}

static uint32_t
winapi_get_mouse_state( int32_t* x, int32_t* y )
{
	*x = winapi.mouse_position[ 0 ];
	*y = winapi.mouse_position[ 1 ];
	return 0;
}

struct ft_window*
winapi_create_window( const struct ft_window_info* info )
{
	ft_window_get_size_impl             = winapi_window_get_size;
	ft_window_get_framebuffer_size_impl = winapi_window_get_framebuffer_size;
	ft_destroy_window_impl              = winapi_destroy_window;
	ft_window_show_cursor_impl          = winapi_window_show_cursor;
	ft_window_should_close_impl         = winapi_window_should_close;
	ft_poll_events_impl                 = winapi_poll_events;
	ft_get_keyboard_state_impl          = winapi_get_keyboard_state;
	ft_get_mouse_state_impl             = winapi_get_mouse_state;
	ft_window_get_vulkan_instance_extensions_impl =
	    winapi_window_get_vulkan_instance_extensions;
	ft_window_create_vulkan_surface_impl = winapi_window_create_vulkan_surface;

	memset( &winapi, 0, sizeof( winapi ) );
	struct ft_window* r = malloc( sizeof( struct ft_window ) );

	HINSTANCE h_instance = GetModuleHandle( NULL );

	WNDCLASS wc = {
	    .lpfnWndProc   = wnd_proc,
	    .hInstance     = h_instance,
	    .lpszClassName = FT_WINDOW_CLASS,
	};

	RegisterClass( &wc );

	WCHAR* title;
	int    size = MultiByteToWideChar( CP_ACP, 0, info->title, -1, NULL, 0 );
	title       = malloc( size * sizeof( WCHAR ) );
	MultiByteToWideChar( CP_ACP, 0, info->title, -1, ( LPWSTR ) title, size );

	HWND hwnd = CreateWindowEx( 0,
	                            FT_WINDOW_CLASS,
	                            title,
	                            WS_OVERLAPPEDWINDOW,
	                            info->x,
	                            info->y,
	                            info->width,
	                            info->height,
	                            NULL,
	                            NULL,
	                            h_instance,
	                            r );

	free( title );

	if ( hwnd == NULL )
	{
		FT_ERROR( "failed to create window" );
		return NULL;
	}

	ShowWindow( hwnd, SW_SHOW );
	UpdateWindow( hwnd );
	SetFocus( hwnd );

	r->window       = hwnd;
	r->width        = info->width;
	r->height       = info->height;
	r->should_close = false;

	return r;
}

#endif
