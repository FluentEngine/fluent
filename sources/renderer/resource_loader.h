#pragma once

#include "renderer/renderer_backend.h"

void
resource_loader_init( const struct Device*, u64 staging_buffer_size );

void
resource_loader_shutdown( void );

void
resource_loader_begin_recording( void );

void
resource_loader_end_recording( void );

void
upload_buffer( struct Buffer* buffer, u64 offset, u64 size, const void* data );

void*
begin_upload_buffer( struct Buffer* buffer );

void
end_upload_buffer( struct Buffer* buffer );

void
upload_image( struct Image* image, u64 size, const void* data );

void
reset_staging_buffer( void );
