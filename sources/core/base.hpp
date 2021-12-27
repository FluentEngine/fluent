#pragma once
#ifdef FLUENT_DEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cstddef>
#include <cstdint>

using byte = int8_t;
using ubyte = uint8_t;
using i16 = int16_t;
using u16 = uint16_t;
using i32 = int32_t;
using u32 = uint32_t;
using f32 = float;
using f64 = double;

#include "core/log.hpp"