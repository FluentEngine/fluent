#pragma once

#ifdef FT_MOVED_C
#include "renderer/renderer_backend.h"

class ResourceLoader
{
	struct StagingBuffer
	{
		u64            offset = 0;
		struct Buffer* buffer = nullptr;
	};

private:
	static const struct Device*  device;
	static struct Queue*         queue;
	static struct CommandPool*   command_pool;
	static struct CommandBuffer* cmd;
	static b32                   is_recording;
	static StagingBuffer         staging_buffer;

public:
	static void
	init( const struct Device* device, u64 staging_buffer_size );

	static void
	shutdown();

	static void
	begin_recording();
	static void
	end_recording();

	static void
	upload_buffer( struct Buffer* buffer,
	               u64            offset,
	               u64            size,
	               const void*    data );

	static void*
	begin_upload_buffer( struct Buffer* buffer );
	static void
	end_upload_buffer( struct Buffer* buffer );

	static void
	upload_image( struct Image* image, u64 size, const void* data );
	static void
	reset_staging_buffer();
};

#endif
