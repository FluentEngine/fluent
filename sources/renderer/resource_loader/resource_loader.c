#include "resource_loader.h"

struct StagingBuffer
{
	u64            offset;
	struct Buffer* buffer;
};

struct ResourceLoader
{
	const struct Device*  device;
	struct Queue*         queue;
	struct CommandPool*   command_pool;
	struct CommandBuffer* cmd;
	b32                   is_recording;
	struct StagingBuffer  staging_buffer;
};

static struct ResourceLoader loader;

static inline b32
need_staging( struct Buffer* buffer )
{
	return !( buffer->memory_usage == FT_MEMORY_USAGE_CPU_ONLY ||
	          buffer->memory_usage == FT_MEMORY_USAGE_CPU_TO_GPU );
}

void
resource_loader_init( const struct Device* device, u64 staging_buffer_size )
{
	loader.device                = device;
	loader.is_recording          = 0;
	loader.staging_buffer.offset = 0;

	struct QueueInfo queue_info = {
		.queue_type = FT_QUEUE_TYPE_GRAPHICS,
	};
	create_queue( device, &queue_info, &loader.queue );

	struct CommandPoolInfo cmd_pool_info = { .queue = loader.queue };
	create_command_pool( device, &cmd_pool_info, &loader.command_pool );

	create_command_buffers( device, loader.command_pool, 1, &loader.cmd );

	struct BufferInfo staging_buffer_info = {
		.memory_usage = FT_MEMORY_USAGE_CPU_TO_GPU,
		.size         = staging_buffer_size,
	};
	loader.staging_buffer.offset = 0;
	create_buffer( device,
	               &staging_buffer_info,
	               &loader.staging_buffer.buffer );
	map_memory( device, loader.staging_buffer.buffer );
}

void
resource_loader_shutdown()
{
	unmap_memory( loader.device, loader.staging_buffer.buffer );
	destroy_buffer( loader.device, loader.staging_buffer.buffer );
	destroy_command_buffers( loader.device,
	                         loader.command_pool,
	                         1,
	                         &loader.cmd );
	destroy_command_pool( loader.device, loader.command_pool );
	destroy_queue( loader.queue );
}

void
upload_buffer( struct Buffer* buffer, u64 offset, u64 size, const void* data )
{
	FT_ASSERT( buffer );
	FT_ASSERT( data );
	FT_ASSERT( size + offset <= buffer->size );

	if ( need_staging( buffer ) )
	{
		memcpy( ( u8* ) loader.staging_buffer.buffer->mapped_memory +
		            loader.staging_buffer.offset,
		        data,
		        size );

		b32 need_end_record = !loader.is_recording;
		if ( need_end_record )
		{
			begin_command_buffer( loader.cmd );
		}

		cmd_copy_buffer( loader.cmd,
		                 loader.staging_buffer.buffer,
		                 loader.staging_buffer.offset,
		                 buffer,
		                 offset,
		                 size );

		loader.staging_buffer.offset += size;

		if ( need_end_record )
		{
			end_command_buffer( loader.cmd );
			immediate_submit( loader.queue, loader.cmd );
		}
	}
	else
	{
		map_memory( loader.device, buffer );
		memcpy( ( u8* ) buffer->mapped_memory + offset, data, size );
		unmap_memory( loader.device, buffer );
	}
}

void*
begin_upload_buffer( struct Buffer* buffer )
{
	FT_ASSERT( buffer );
	if ( need_staging( buffer ) )
	{
		// TODO:
		return NULL;
	}
	else
	{
		map_memory( loader.device, buffer );
		return buffer->mapped_memory;
	}
}

void
end_upload_buffer( struct Buffer* buffer )
{
	FT_ASSERT( buffer );
	if ( need_staging( buffer ) )
	{
		// TODO:
	}
	else
	{
		unmap_memory( loader.device, buffer );
	}
}

void
upload_image( struct Image* image, u64 size, const void* data )
{
	FT_ASSERT( loader.staging_buffer.offset + size <= loader.staging_buffer.buffer->size );

	memcpy( ( u8* ) loader.staging_buffer.buffer->mapped_memory +
	            loader.staging_buffer.offset,
	        data,
	        size );

	b32 need_end_record = !loader.is_recording;
	if ( need_end_record )
	{
		begin_command_buffer( loader.cmd );
	}

	struct ImageBarrier barrier = {};
	barrier.image               = image;
	barrier.old_state           = FT_RESOURCE_STATE_UNDEFINED;
	barrier.new_state           = FT_RESOURCE_STATE_TRANSFER_DST;
	cmd_barrier( loader.cmd, 0, NULL, 0, NULL, 1, &barrier );

	cmd_copy_buffer_to_image( loader.cmd,
	                          loader.staging_buffer.buffer,
	                          loader.staging_buffer.offset,
	                          image );

	barrier.old_state = FT_RESOURCE_STATE_TRANSFER_DST;
	barrier.new_state = FT_RESOURCE_STATE_SHADER_READ_ONLY;
	cmd_barrier( loader.cmd, 0, NULL, 0, NULL, 1, &barrier );

	loader.staging_buffer.offset += size;

	if ( need_end_record )
	{
		end_command_buffer( loader.cmd );
		immediate_submit( loader.queue, loader.cmd );
	}
}

void
reset_staging_buffer()
{
	loader.staging_buffer.offset = 0;
}

void
resource_loader_begin_recording()
{
	begin_command_buffer( loader.cmd );
	loader.is_recording = 1;
}

void
resource_loader_end_recording()
{
	loader.is_recording = 0;
	end_command_buffer( loader.cmd );
	immediate_submit( loader.queue, loader.cmd );
}
