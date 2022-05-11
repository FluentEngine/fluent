#include "renderer/resource_loader.hpp"

namespace fluent
{
const Device*                 ResourceLoader::m_device       = nullptr;
Queue*                        ResourceLoader::m_queue        = nullptr;
CommandPool*                  ResourceLoader::m_command_pool = nullptr;
CommandBuffer*                ResourceLoader::m_cmd          = nullptr;
ResourceLoader::StagingBuffer ResourceLoader::m_staging_buffer;
b32                           ResourceLoader::m_is_recording = false;

static b32
need_staging( Buffer* buffer )
{
	return !( buffer->memory_usage == MemoryUsage::CPU_ONLY ||
	          buffer->memory_usage == MemoryUsage::CPU_TO_GPU );
}

void
ResourceLoader::init( const Device* device, u64 staging_buffer_size )
{
	m_device = device;

	QueueInfo queue_info {};
	queue_info.queue_type = QueueType::GRAPHICS;
	create_queue( m_device, &queue_info, &m_queue );

	CommandPoolInfo cmd_pool_info {};
	cmd_pool_info.queue = m_queue;
	create_command_pool( m_device, &cmd_pool_info, &m_command_pool );

	create_command_buffers( m_device, m_command_pool, 1, &m_cmd );

	BufferInfo staging_buffer_info {};
	staging_buffer_info.memory_usage = MemoryUsage::CPU_TO_GPU;
	staging_buffer_info.size         = staging_buffer_size;
	m_staging_buffer.offset          = 0;
	create_buffer( m_device, &staging_buffer_info, &m_staging_buffer.buffer );
	map_memory( m_device, m_staging_buffer.buffer );
}

void
ResourceLoader::shutdown()
{
	unmap_memory( m_device, m_staging_buffer.buffer );
	destroy_buffer( m_device, m_staging_buffer.buffer );
	destroy_command_buffers( m_device, m_command_pool, 1, &m_cmd );
	destroy_command_pool( m_device, m_command_pool );
	destroy_queue( m_queue );
}

void
ResourceLoader::upload_buffer( Buffer*     buffer,
                               u64         offset,
                               u64         size,
                               const void* data )
{
	FT_ASSERT( buffer );
	FT_ASSERT( data );
	FT_ASSERT( size + offset <= buffer->size );

	if ( need_staging( buffer ) )
	{
		std::memcpy( ( u8* ) m_staging_buffer.buffer->mapped_memory +
		                 m_staging_buffer.offset,
		             data,
		             size );

		b32 need_end_record = !m_is_recording;
		if ( need_end_record )
		{
			begin_command_buffer( m_cmd );
		}

		cmd_copy_buffer( m_cmd,
		                 m_staging_buffer.buffer,
		                 m_staging_buffer.offset,
		                 buffer,
		                 offset,
		                 size );

		m_staging_buffer.offset += size;

		if ( need_end_record )
		{
			end_command_buffer( m_cmd );
			immediate_submit( m_queue, m_cmd );
		}
	}
	else
	{
		map_memory( m_device, buffer );
		std::memcpy( ( u8* ) buffer->mapped_memory + offset, data, size );
		unmap_memory( m_device, buffer );
	}
}

void*
ResourceLoader::begin_upload_buffer( Buffer* buffer )
{
	FT_ASSERT( buffer );
	if ( need_staging( buffer ) )
	{
		// TODO:
		return nullptr;
	}
	else
	{
		map_memory( m_device, buffer );
		return buffer->mapped_memory;
	}
}

void
ResourceLoader::end_upload_buffer( Buffer* buffer )
{
	FT_ASSERT( buffer );
	if ( need_staging( buffer ) )
	{
		// TODO:
	}
	else
	{
		unmap_memory( m_device, buffer );
	}
}

void
ResourceLoader::upload_image( Image* image, u64 size, const void* data )
{
	FT_ASSERT( m_staging_buffer.offset + size <=
	           m_staging_buffer.buffer->size );

	std::memcpy( ( u8* ) m_staging_buffer.buffer->mapped_memory +
	                 m_staging_buffer.offset,
	             data,
	             size );

	b32 need_end_record = !m_is_recording;
	if ( need_end_record )
	{
		begin_command_buffer( m_cmd );
	}

	ImageBarrier barrier {};
	barrier.image     = image;
	barrier.src_queue = m_queue;
	barrier.dst_queue = m_queue;
	barrier.old_state = ResourceState::UNDEFINED;
	barrier.new_state = ResourceState::TRANSFER_DST;
	cmd_barrier( m_cmd, 0, nullptr, 0, nullptr, 1, &barrier );
	cmd_copy_buffer_to_image( m_cmd,
	                          m_staging_buffer.buffer,
	                          m_staging_buffer.offset,
	                          image );
	barrier.old_state = ResourceState::TRANSFER_DST;
	barrier.new_state = ResourceState::SHADER_READ_ONLY;
	cmd_barrier( m_cmd, 0, nullptr, 0, nullptr, 1, &barrier );

	m_staging_buffer.offset += size;

	if ( need_end_record )
	{
		end_command_buffer( m_cmd );
		immediate_submit( m_queue, m_cmd );
	}
}

void
ResourceLoader::reset_staging_buffer()
{
	m_staging_buffer.offset = 0;
}

void
ResourceLoader::begin_recording()
{
	begin_command_buffer( m_cmd );
	m_is_recording = true;
}

void
ResourceLoader::end_recording()
{
	m_is_recording = false;
	end_command_buffer( m_cmd );
	immediate_submit( m_queue, m_cmd );
}

} // namespace fluent
