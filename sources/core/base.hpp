#pragma once

#ifdef FLUENT_DEBUG
#undef NDEBUG
#endif

#include <string>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

using i8  = int8_t;
using u8  = uint8_t;
using i16 = int16_t;
using u16 = uint16_t;
using i32 = int32_t;
using u32 = uint32_t;
using i64 = int64_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;
using b32 = bool;

#define MAKE_ENUM_FLAG(TYPE, ENUM_TYPE)                                                                                \
    static inline ENUM_TYPE operator|(ENUM_TYPE a, ENUM_TYPE b)                                                        \
    {                                                                                                                  \
        return ( ENUM_TYPE ) (( TYPE ) (a) | ( TYPE ) (b));                                                            \
    }                                                                                                                  \
    static inline ENUM_TYPE operator&(ENUM_TYPE a, ENUM_TYPE b)                                                        \
    {                                                                                                                  \
        return ( ENUM_TYPE ) (( TYPE ) (a) & ( TYPE ) (b));                                                            \
    }                                                                                                                  \
    static inline ENUM_TYPE operator|=(ENUM_TYPE& a, ENUM_TYPE b)                                                      \
    {                                                                                                                  \
        return a = (a | b);                                                                                            \
    }                                                                                                                  \
    static inline ENUM_TYPE operator&=(ENUM_TYPE& a, ENUM_TYPE b)                                                      \
    {                                                                                                                  \
        return a = (a & b);                                                                                            \
    }

template <typename T>
using Scope = std::unique_ptr<T>;

template <typename T>
using Ref = std::shared_ptr<T>;

template <typename T, typename... Args>
Scope<T> CreateScope(Args&&... args)
{
    return std::make_unique<T>(args...);
}

template <typename T, typename... Args>
Ref<T> CreateRef(Args&&... args)
{
    return std::make_shared<T>(args...);
}

#include "core/log.hpp"
#include "core/assert.hpp"