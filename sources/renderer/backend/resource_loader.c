#include "renderer_backend.h"

struct StagingBuffer
{
	uint64_t          offset;
	struct ft_buffer* buffer;
};

struct ResourceLoader
{
	const struct ft_device*   device;
	struct ft_queue*          queue;
	struct ft_command_pool*   command_pool;
	struct ft_command_buffer* cmd;
	bool                      is_recording;
	struct StagingBuffer      staging_buffer;
	uint64_t                  last_batch_write_size;
};

static struct ResourceLoader loader;

FT_INLINE bool
need_staging( struct ft_buffer* buffer )
{
	return !( buffer->memory_usage == FT_MEMORY_USAGE_CPU_ONLY ||
	          buffer->memory_usage == FT_MEMORY_USAGE_CPU_TO_GPU );
}

void
resource_loader_init( const struct ft_device* device,
                      uint64_t                staging_buffer_size )
{
	loader.device                = device;
	loader.is_recording          = 0;
	loader.staging_buffer.offset = 0;
	loader.last_batch_write_size = 0;

	struct ft_queue_info queue_info = {
	    .queue_type = FT_QUEUE_TYPE_GRAPHICS,
	};
	ft_create_queue( device, &queue_info, &loader.queue );

	struct ft_command_pool_info cmd_pool_info = { .queue = loader.queue };
	ft_create_command_pool( device, &cmd_pool_info, &loader.command_pool );

	ft_create_command_buffers( device, loader.command_pool, 1, &loader.cmd );

	struct ft_buffer_info staging_buffer_info = {
	    .memory_usage    = FT_MEMORY_USAGE_CPU_TO_GPU,
	    .size            = staging_buffer_size,
	    .descriptor_type = 0,
	};
	loader.staging_buffer.offset = 0;
	ft_create_buffer( device,
	                  &staging_buffer_info,
	                  &loader.staging_buffer.buffer );
	ft_map_memory( device, loader.staging_buffer.buffer );
}

void
resource_loader_shutdown()
{
	ft_unmap_memory( loader.device, loader.staging_buffer.buffer );
	ft_destroy_buffer( loader.device, loader.staging_buffer.buffer );
	ft_destroy_command_buffers( loader.device,
	                            loader.command_pool,
	                            1,
	                            &loader.cmd );
	ft_destroy_command_pool( loader.device, loader.command_pool );
	ft_destroy_queue( loader.queue );
}

void
ft_upload_buffer( struct ft_buffer* buffer,
                  uint64_t          offset,
                  uint64_t          size,
                  const void*       data )
{
	FT_ASSERT( buffer );
	FT_ASSERT( data );
	FT_ASSERT( size + offset <= buffer->size );

	if ( need_staging( buffer ) )
	{
		memcpy( ( uint8_t* ) loader.staging_buffer.buffer->mapped_memory +
		            loader.staging_buffer.offset,
		        data,
		        size );

		bool need_end_record = !loader.is_recording;
		if ( need_end_record )
		{
			ft_begin_command_buffer( loader.cmd );
		}

		ft_cmd_copy_buffer( loader.cmd,
		                    loader.staging_buffer.buffer,
		                    loader.staging_buffer.offset,
		                    buffer,
		                    offset,
		                    size );

		loader.staging_buffer.offset += size;

		if ( need_end_record )
		{
			ft_end_command_buffer( loader.cmd );
			ft_immediate_submit( loader.queue, loader.cmd );
			loader.staging_buffer.offset -= size;
		}
		else
		{
			loader.last_batch_write_size += size;
		}
	}
	else
	{
		ft_map_memory( loader.device, buffer );
		memcpy( ( uint8_t* ) buffer->mapped_memory + offset, data, size );
		ft_unmap_memory( loader.device, buffer );
	}
}

void
ft_upload_image( struct ft_image*                   image,
                 const struct ft_upload_image_info* info )
{
	FT_ASSERT( info );
	FT_ASSERT( info->data );
	size_t upload_size =
	    info->width * info->height * ft_format_size_bytes( image->format );
	FT_ASSERT( loader.staging_buffer.offset + upload_size <=
	           loader.staging_buffer.buffer->size );

	memcpy( loader.staging_buffer.buffer->mapped_memory +
	            loader.staging_buffer.offset,
	        info->data,
	        upload_size );

	bool need_end_record = !loader.is_recording;
	if ( need_end_record )
	{
		ft_begin_command_buffer( loader.cmd );
	}

	struct ft_image_barrier barrier = { 0 };
	barrier.image                   = image;
	barrier.old_state               = FT_RESOURCE_STATE_UNDEFINED;
	barrier.new_state               = FT_RESOURCE_STATE_TRANSFER_DST;
	ft_cmd_barrier( loader.cmd, 0, NULL, 0, NULL, 1, &barrier );

	struct ft_buffer_image_copy copy = {
	    .buffer_offset = loader.staging_buffer.offset,
	    .width         = info->width,
	    .height        = info->height,
	    .mip_level     = info->mip_level,
	};

	ft_cmd_copy_buffer_to_image( loader.cmd,
	                             loader.staging_buffer.buffer,
	                             image,
	                             &copy );

	barrier.old_state = FT_RESOURCE_STATE_TRANSFER_DST;
	barrier.new_state = FT_RESOURCE_STATE_SHADER_READ_ONLY;
	ft_cmd_barrier( loader.cmd, 0, NULL, 0, NULL, 1, &barrier );

	loader.staging_buffer.offset += upload_size;

	if ( need_end_record )
	{
		ft_end_command_buffer( loader.cmd );
		ft_immediate_submit( loader.queue, loader.cmd );
		loader.staging_buffer.offset -= upload_size;
	}
	else
	{
		loader.last_batch_write_size += upload_size;
	}
}

void
ft_begin_upload_batch()
{
	ft_begin_command_buffer( loader.cmd );
	loader.is_recording = 1;
}

void
ft_end_upload_batch()
{
	loader.is_recording = 0;
	loader.staging_buffer.offset -= loader.last_batch_write_size;
	loader.last_batch_write_size = 0;
	ft_end_command_buffer( loader.cmd );
	ft_immediate_submit( loader.queue, loader.cmd );
}
