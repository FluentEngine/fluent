#pragma once

#include "base/base.h"

FT_API void*
ft_read_file_binary( const char* filename, uint64_t* size );

FT_API void
ft_free_file_data( void* data );

FT_API void*
ft_read_image_from_file( const char* filename,
                         uint32_t*   width,
                         uint32_t*   height );

FT_API void
ft_free_image_data( void* );
