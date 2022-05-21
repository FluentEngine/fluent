#ifdef FT_MOVED_C
#include "renderer/resource_loader.hpp"

const struct Device*          ResourceLoader::device       = nullptr;
struct Queue*                 ResourceLoader::queue        = nullptr;
struct CommandPool*           ResourceLoader::command_pool = nullptr;
struct CommandBuffer*         ResourceLoader::cmd          = nullptr;
ResourceLoader::StagingBuffer ResourceLoader::staging_buffer;
b32                           ResourceLoader::is_recording = false;

static b32
need_staging( struct Buffer* buffer )
{
	return !( buffer->memory_usage == FT_MEMORY_USAGE_CPU_ONLY ||
	          buffer->memory_usage == FT_MEMORY_USAGE_CPU_TO_GPU );
}

void
ResourceLoader::init( const struct Device* _device, u64 staging_buffer_size )
{
	device = _device;

	QueueInfo queue_info {};
	queue_info.queue_type = FT_QUEUE_TYPE_GRAPHICS;
	create_queue( device, &queue_info, &queue );

	CommandPoolInfo cmd_pool_info {};
	cmd_pool_info.queue = queue;
	create_command_pool( device, &cmd_pool_info, &command_pool );

	create_command_buffers( device, command_pool, 1, &cmd );

	BufferInfo staging_buffer_info {};
	staging_buffer_info.memory_usage = FT_MEMORY_USAGE_CPU_TO_GPU;
	staging_buffer_info.size         = staging_buffer_size;
	staging_buffer.offset            = 0;
	create_buffer( device, &staging_buffer_info, &staging_buffer.buffer );
	map_memory( device, staging_buffer.buffer );
}

void
ResourceLoader::shutdown()
{
	unmap_memory( device, staging_buffer.buffer );
	destroy_buffer( device, staging_buffer.buffer );
	destroy_command_buffers( device, command_pool, 1, &cmd );
	destroy_command_pool( device, command_pool );
	destroy_queue( queue );
}

void
ResourceLoader::upload_buffer( struct Buffer* buffer,
                               u64            offset,
                               u64            size,
                               const void*    data )
{
	FT_ASSERT( buffer );
	FT_ASSERT( data );
	FT_ASSERT( size + offset <= buffer->size );

	if ( need_staging( buffer ) )
	{
		memcpy( ( u8* ) staging_buffer.buffer->mapped_memory +
		            staging_buffer.offset,
		        data,
		        size );

		b32 need_end_record = !is_recording;
		if ( need_end_record )
		{
			begin_command_buffer( cmd );
		}

		cmd_copy_buffer( cmd,
		                 staging_buffer.buffer,
		                 staging_buffer.offset,
		                 buffer,
		                 offset,
		                 size );

		staging_buffer.offset += size;

		if ( need_end_record )
		{
			end_command_buffer( cmd );
			immediate_submit( queue, cmd );
		}
	}
	else
	{
		map_memory( device, buffer );
		memcpy( ( u8* ) buffer->mapped_memory + offset, data, size );
		unmap_memory( device, buffer );
	}
}

void*
ResourceLoader::begin_upload_buffer( struct Buffer* buffer )
{
	FT_ASSERT( buffer );
	if ( need_staging( buffer ) )
	{
		// TODO:
		return nullptr;
	}
	else
	{
		map_memory( device, buffer );
		return buffer->mapped_memory;
	}
}

void
ResourceLoader::end_upload_buffer( struct Buffer* buffer )
{
	FT_ASSERT( buffer );
	if ( need_staging( buffer ) )
	{
		// TODO:
	}
	else
	{
		unmap_memory( device, buffer );
	}
}

void
ResourceLoader::upload_image( struct Image* image, u64 size, const void* data )
{
	FT_ASSERT( staging_buffer.offset + size <= staging_buffer.buffer->size );

	memcpy( ( u8* ) staging_buffer.buffer->mapped_memory +
	            staging_buffer.offset,
	        data,
	        size );

	b32 need_end_record = !is_recording;
	if ( need_end_record )
	{
		begin_command_buffer( cmd );
	}

	ImageBarrier barrier {};
	barrier.image     = image;
	barrier.src_queue = queue;
	barrier.dst_queue = queue;
	barrier.old_state = FT_RESOURCE_STATE_UNDEFINED;
	barrier.new_state = FT_RESOURCE_STATE_TRANSFER_DST;
	cmd_barrier( cmd, 0, nullptr, 0, nullptr, 1, &barrier );
	cmd_copy_buffer_to_image( cmd,
	                          staging_buffer.buffer,
	                          staging_buffer.offset,
	                          image );
	barrier.old_state = FT_RESOURCE_STATE_TRANSFER_DST;
	barrier.new_state = FT_RESOURCE_STATE_SHADER_READ_ONLY;
	cmd_barrier( cmd, 0, nullptr, 0, nullptr, 1, &barrier );

	staging_buffer.offset += size;

	if ( need_end_record )
	{
		end_command_buffer( cmd );
		immediate_submit( queue, cmd );
	}
}

void
ResourceLoader::reset_staging_buffer()
{
	staging_buffer.offset = 0;
}

void
ResourceLoader::begin_recording()
{
	begin_command_buffer( cmd );
	is_recording = true;
}

void
ResourceLoader::end_recording()
{
	is_recording = false;
	end_command_buffer( cmd );
	immediate_submit( queue, cmd );
}
#endif
