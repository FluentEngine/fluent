#pragma once

#include <cstdint>

namespace fluent
{
using ButtonCode = u16;

namespace Button
{
    enum : ButtonCode
    {
        Left   = 1,
        Middle = 2,
        Right  = 3,
        Last   = 4
    };
} // namespace Button
} // namespace fluent
