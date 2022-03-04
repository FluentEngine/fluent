#pragma once

#include "core/base.hpp"

namespace fluent
{

using KeyCode = u16;

namespace Key
{
    enum : KeyCode
    {
        Space      = 44,
        Apostrophe = 52, /* ' */
        Comma      = 54, /* , */
        Minus      = 45, /* - */
        Period     = 55, /* . */
        Slash      = 56, /* / */

        Semicolon = 51, /* ; */
        Equal     = 46, /* = */

        A = 4,
        B = 5,
        C = 6,
        D = 7,
        E = 8,
        F = 9,
        G = 10,
        H = 11,
        I = 12,
        J = 13,
        K = 14,
        L = 15,
        M = 16,
        N = 17,
        O = 18,
        P = 19,
        Q = 20,
        R = 21,
        S = 22,
        T = 23,
        U = 24,
        V = 25,
        W = 26,
        X = 27,
        Y = 28,
        Z = 29,

        LeftBracket  = 47, /* [ */
        Backslash    = 49, /* \ */
        RightBracket = 48, /* ] */
        GraveAccent  = 53, /* ` */

        /* Function keys */
        Escape      = 41,
        Enter       = 40,
        Tab         = 43,
        Backspace   = 42,
        Insert      = 73,
        Delete      = 76,
        Right       = 79,
        Left        = 80,
        Down        = 81,
        Up          = 82,
        PageUp      = 75,
        PageDown    = 78,
        Home        = 74,
        End         = 77,
        CapsLock    = 57,
        ScrollLock  = 71,
        NumLock     = 83,
        PrintScreen = 70,
        Pause       = 72,
        F1          = 58,
        F2          = 59,
        F3          = 60,
        F4          = 61,
        F5          = 62,
        F6          = 63,
        F7          = 64,
        F8          = 65,
        F9          = 66,
        F10         = 67,
        F11         = 68,
        F12         = 69,
        F13         = 70,
        F14         = 71,
        F15         = 72,
        F16         = 73,
        F17         = 74,
        F18         = 75,
        F19         = 76,
        F20         = 77,
        F21         = 78,
        F22         = 79,
        F23         = 80,
        F24         = 81,
        F25         = 82,

        /* Keypad */
        KP0        = 98,
        KP1        = 89,
        KP2        = 90,
        KP3        = 91,
        KP4        = 92,
        KP5        = 93,
        KP6        = 94,
        KP7        = 95,
        KP8        = 96,
        KP9        = 97,
        KPDecimal  = 220,
        KPDivide   = 84,
        KPMultiply = 213,
        KPSubtract = 212,
        KPAdd      = 211,
        KPEnter    = 88,
        KPEqual    = 103,

        LeftShift    = 225,
        LeftControl  = 224,
        LeftAlt      = 226,
        LeftSuper    = 101,
        RightShift   = 229,
        RightControl = 228,
        RightAlt     = 230,
        RightSuper   = 101,
        Menu         = 348,
        Last         = 350
    };
} // namespace Key
} // namespace fluent
