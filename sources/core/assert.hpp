#pragma once

#include <cassert>

#ifdef FLUENT_DEBUG
#    define FT_ASSERT( x ) assert( x )
#else
#    define FT_ASSERT( x )
#endif