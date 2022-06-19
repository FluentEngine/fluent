#pragma once

#include "base/base.h"

enum ft_key_code
{
	FT_KEY_SPACE      = 44,
	FT_KEY_APOSTROPHE = 52, /* ' */
	FT_KEY_COMMA      = 54, /* , */
	FT_KEY_MINUS      = 45, /* - */
	FT_KEY_PERIOD     = 55, /* . */
	FT_KEY_SLASH      = 56, /* / */

	FT_KEY_SEMICOLON = 51, /* ; */
	FT_KEY_EQUAL     = 46, /* = */

	FT_KEY_A = 4,
	FT_KEY_B = 5,
	FT_KEY_C = 6,
	FT_KEY_D = 7,
	FT_KEY_E = 8,
	FT_KEY_F = 9,
	FT_KEY_G = 10,
	FT_KEY_H = 11,
	FT_KEY_I = 12,
	FT_KEY_J = 13,
	FT_KEY_K = 14,
	FT_KEY_L = 15,
	FT_KEY_M = 16,
	FT_KEY_N = 17,
	FT_KEY_O = 18,
	FT_KEY_P = 19,
	FT_KEY_Q = 20,
	FT_KEY_R = 21,
	FT_KEY_S = 22,
	FT_KEY_T = 23,
	FT_KEY_U = 24,
	FT_KEY_V = 25,
	FT_KEY_W = 26,
	FT_KEY_X = 27,
	FT_KEY_Y = 28,
	FT_KEY_Z = 29,

	FT_KEY_LEFT_BRACKET  = 47, /* [ */
	FT_KEY_BACKSLASH     = 49, /* \ */
	FT_KEY_RIGHT_BRACKET = 48, /* ] */
	FT_KEY_GRAVE_ACCENT  = 53, /* ` */

	/* Function keys */
	FT_KEY_ESCAPE       = 41,
	FT_KEY_ENTER        = 40,
	FT_KEY_TAB          = 43,
	FT_KEY_BACKSPACE    = 42,
	FT_KEY_INSERT       = 73,
	FT_KEY_DELETE       = 76,
	FT_KEY_RIGHT        = 79,
	FT_KEY_LEFT         = 80,
	FT_KEY_DOWN         = 81,
	FT_KEY_UP           = 82,
	FT_KEY_PAGE_UP      = 75,
	FT_KEY_PAGE_DOWN    = 78,
	FT_KEY_HOME         = 74,
	FT_KEY_END          = 77,
	FT_KEY_CAPS_LOCK    = 57,
	FT_KEY_SCROLL_LOCK  = 71,
	FT_KEY_NUM_LOCK     = 83,
	FT_KEY_PRINT_SCREEN = 70,
	FT_KEY_PAUSE        = 72,
	FT_KEY_F1           = 58,
	FT_KEY_F2           = 59,
	FT_KEY_F3           = 60,
	FT_KEY_F4           = 61,
	FT_KEY_F5           = 62,
	FT_KEY_F6           = 63,
	FT_KEY_F7           = 64,
	FT_KEY_F8           = 65,
	FT_KEY_F9           = 66,
	FT_KEY_F10          = 67,
	FT_KEY_F11          = 68,
	FT_KEY_F12          = 69,
	FT_KEY_F13          = 70,
	FT_KEY_F14          = 71,
	FT_KEY_F15          = 72,
	FT_KEY_F16          = 73,
	FT_KEY_F17          = 74,
	FT_KEY_F18          = 75,
	FT_KEY_F19          = 76,
	FT_KEY_F20          = 77,
	FT_KEY_F21          = 78,
	FT_KEY_F22          = 79,
	FT_KEY_F23          = 80,
	FT_KEY_F24          = 81,
	FT_KEY_F25          = 82,

	/* Keypad */
	FT_KEY_KP0          = 98,
	FT_KEY_KP1          = 89,
	FT_KEY_KP2          = 90,
	FT_KEY_KP3          = 91,
	FT_KEY_KP4          = 92,
	FT_KEY_KP5          = 93,
	FT_KEY_KP6          = 94,
	FT_KEY_KP7          = 95,
	FT_KEY_KP8          = 96,
	FT_KEY_KP9          = 97,
	FT_KEY_KP_DECIMAL   = 220,
	FT_KEY_KP_DIVIDE    = 84,
	FT_KEY_KP_MULTIPLY  = 213,
	FT_KEY_KP_SUBSTRACT = 212,
	FT_KEY_KP_ADD       = 211,
	FT_KEY_KP_ENTER     = 88,
	FT_KEY_KP_EQUAL     = 103,

	FT_KEY_LEFT_SHIFT    = 225,
	FT_KEY_LEFT_CONTROL  = 224,
	FT_KEY_LEFT_ALT      = 226,
	FT_KEY_LEFT_SUPER    = 101,
	FT_KEY_RIGHT_SHIFT   = 229,
	FT_KEY_RIGHT_CONTROL = 228,
	FT_KEY_RIGHT_ALT     = 230,
	FT_KEY_RIGHT_SUPER   = 101,
	FT_KEY_MENU          = 348,
	FT_KEY_COUNT         = 350
};
