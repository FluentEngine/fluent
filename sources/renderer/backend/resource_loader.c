#include "time/timer.h"
#include "thread/thread.h"
#include "renderer_enums_stringifier.h"
#include "renderer_private.h"
#if FT_VULKAN_BACKEND
#include "vulkan/vulkan_backend.h"
#endif
#include "renderer_backend.h"
#include "resource_loader.h"

enum ft_loader_job_type
{
	FT_LOADER_JOB_TYPE_BUFFER_UPLOAD,
	FT_LOADER_JOB_TYPE_IMAGE_UPLOAD,
	FT_LOADER_JOB_TYPE_GENERATE_MIPMAPS,
};

struct ft_loader_job
{
	enum ft_loader_job_type type;
	struct ft_loader_job*   next;
	struct ft_buffer*       staging_buffer;
	union
	{
		struct ft_buffer_upload_job    buffer_upload_job;
		struct ft_image_upload_job     image_upload_job;
		struct ft_generate_mipmaps_job generate_mipmaps_job;
	};
};

struct ft_loader
{
	const struct ft_device*   device;
	struct ft_queue*          queue;
	struct ft_command_pool*   command_pool;
	struct ft_command_buffer* cmd;
	uint32_t                  job_count;
	struct ft_loader_job*     head;
	struct ft_loader_job*     tail;
	struct ft_thread          thread;
	struct ft_mutex           mutex;
	bool                      alive;
};

static struct ft_loader loader;

uint32_t
loader_thread_fun( void* arg );

#if FT_VULKAN_BACKEND
void
vk_generate_mipmaps( struct ft_command_buffer* icmd,
                     struct ft_image*          iimage,
                     enum ft_resource_state    state )
{
	FT_FROM_HANDLE( image, iimage, vk_image );
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );

	if ( state != FT_RESOURCE_STATE_TRANSFER_DST )
	{
		struct ft_image_barrier barrier = {
		    .image     = iimage,
		    .old_state = state,
		    .new_state = FT_RESOURCE_STATE_TRANSFER_DST,
		};

		ft_cmd_barrier( icmd, 0, NULL, 0, NULL, 1, &barrier );
	}

	VkImageMemoryBarrier barrier = {
	    .sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
	    .image                       = image->image,
	    .srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED,
	    .dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED,
	    .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	    .subresourceRange.baseArrayLayer = 0,
	    .subresourceRange.layerCount     = 1,
	    .subresourceRange.levelCount     = 1,
	};

	int32_t  mip_width  = iimage->width;
	int32_t  mip_height = iimage->height;
	uint32_t mip_levels = iimage->mip_levels;

	for ( uint32_t i = 1; i < mip_levels; i++ )
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier( cmd->command_buffer,
		                      VK_PIPELINE_STAGE_TRANSFER_BIT,
		                      VK_PIPELINE_STAGE_TRANSFER_BIT,
		                      0,
		                      0,
		                      NULL,
		                      0,
		                      NULL,
		                      1,
		                      &barrier );

		VkImageBlit blit = {
		    .srcOffsets[ 0 ]               = { 0, 0, 0 },
		    .srcOffsets[ 1 ]               = { mip_width, mip_height, 1 },
		    .srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
		    .srcSubresource.mipLevel       = i - 1,
		    .srcSubresource.baseArrayLayer = 0,
		    .srcSubresource.layerCount     = 1,
		    .dstOffsets[ 0 ]               = { 0, 0, 0 },
		    .dstOffsets[ 1 ] =
		        {
		            mip_width > 1 ? mip_width / 2 : 1,
		            mip_height > 1 ? mip_height / 2 : 1,
		            1,
		        },
		    .dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
		    .dstSubresource.mipLevel       = i,
		    .dstSubresource.baseArrayLayer = 0,
		    .dstSubresource.layerCount     = 1,
		};

		vkCmdBlitImage( cmd->command_buffer,
		                image->image,
		                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		                image->image,
		                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		                1,
		                &blit,
		                VK_FILTER_LINEAR );

		barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier( cmd->command_buffer,
		                      VK_PIPELINE_STAGE_TRANSFER_BIT,
		                      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		                      0,
		                      0,
		                      NULL,
		                      0,
		                      NULL,
		                      1,
		                      &barrier );

		if ( mip_width > 1 )
			mip_width /= 2;
		if ( mip_height > 1 )
			mip_height /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mip_levels - 1;
	barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier( cmd->command_buffer,
	                      VK_PIPELINE_STAGE_TRANSFER_BIT,
	                      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                      0,
	                      0,
	                      NULL,
	                      0,
	                      NULL,
	                      1,
	                      &barrier );
}
#endif

FT_INLINE struct ft_buffer*
create_staging_buffer( uint64_t size )
{
	struct ft_buffer* buffer;

	struct ft_buffer_info info = {
	    .memory_usage = FT_MEMORY_USAGE_CPU_TO_GPU,
	    .size         = size,
	};
	ft_create_buffer( loader.device, &info, &buffer );

	return buffer;
}

void
ft_resource_loader_init( const struct ft_device* device )
{
	memset( &loader, 0, sizeof( loader ) );

	loader.device = device;

	struct ft_queue_info queue_info = {
	    .queue_type = FT_QUEUE_TYPE_GRAPHICS,
	};
	ft_create_queue( device, &queue_info, &loader.queue );

	struct ft_command_pool_info cmd_pool_info = {
	    .queue = loader.queue,
	};
	ft_create_command_pool( device, &cmd_pool_info, &loader.command_pool );

	ft_create_command_buffers( device, loader.command_pool, 1, &loader.cmd );

	loader.alive = true;

	ft_mutex_create( &loader.mutex );
	ft_thread_create( &loader.thread, loader_thread_fun, NULL );
}

void
ft_resource_loader_shutdown()
{
	loader.alive = false;
	ft_thread_join( &loader.thread );
	ft_thread_destroy( &loader.thread );
	ft_mutex_destroy( &loader.mutex );
	ft_destroy_command_buffers( loader.device,
	                            loader.command_pool,
	                            1,
	                            &loader.cmd );
	ft_destroy_command_pool( loader.device, loader.command_pool );
	ft_destroy_queue( loader.queue );
}

FT_INLINE void
add_job( const struct ft_loader_job* job )
{
	struct ft_loader_job* tmp = malloc( sizeof( struct ft_loader_job ) );
	memcpy( tmp, job, sizeof( struct ft_loader_job ) );

	while ( !ft_mutex_try_lock( &loader.mutex ) ) {}

	if ( loader.head == NULL )
	{
		loader.head = tmp;
		loader.tail = tmp;
	}
	else
	{
		loader.tail->next = tmp;
		loader.tail       = loader.tail->next;
	}

	loader.job_count++;

	ft_mutex_unlock( &loader.mutex );
}

FT_INLINE void
complete_job( struct ft_command_buffer* cmd, const struct ft_loader_job* j )
{
	switch ( j->type )
	{
	case FT_LOADER_JOB_TYPE_BUFFER_UPLOAD:
	{
		const struct ft_buffer_upload_job* job = &j->buffer_upload_job;

		ft_cmd_copy_buffer( cmd,
		                    j->staging_buffer,
		                    0,
		                    job->buffer,
		                    job->offset,
		                    job->size );
		break;
	}
	case FT_LOADER_JOB_TYPE_IMAGE_UPLOAD:
	{
		const struct ft_image_upload_job* job = &j->image_upload_job;

		struct ft_image_barrier barrier;
		memset( &barrier, 0, sizeof( struct ft_image_barrier ) );
		barrier.image     = job->image;
		barrier.old_state = FT_RESOURCE_STATE_UNDEFINED;
		barrier.new_state = FT_RESOURCE_STATE_TRANSFER_DST;
		ft_cmd_barrier( loader.cmd, 0, NULL, 0, NULL, 1, &barrier );

		struct ft_buffer_image_copy copy = {
		    .buffer_offset = 0,
		    .width         = job->width,
		    .height        = job->height,
		    .mip_level     = job->mip_level,
		};

		ft_cmd_copy_buffer_to_image( cmd,
		                             j->staging_buffer,
		                             job->image,
		                             &copy );

		barrier.old_state = FT_RESOURCE_STATE_TRANSFER_DST;
		barrier.new_state = FT_RESOURCE_STATE_SHADER_READ_ONLY;
		ft_cmd_barrier( cmd, 0, NULL, 0, NULL, 1, &barrier );
		break;
	}
	case FT_LOADER_JOB_TYPE_GENERATE_MIPMAPS:
	{
		const struct ft_generate_mipmaps_job* job = &j->generate_mipmaps_job;

		switch ( loader.device->api )
		{
#if FT_VULKAN_BACKEND
		case FT_RENDERER_API_VULKAN:
			vk_generate_mipmaps( cmd, job->image, job->state );
			break;
#endif
		default:
			FT_WARN( "mipmap generation not implemented for %s",
			         ft_renderer_api_to_string( loader.device->api ) );
			break;
		}
		break;
	}
	}
}

uint32_t
loader_thread_fun( void* arg )
{
	FT_UNUSED( arg );

	while ( loader.alive )
	{
		if ( loader.job_count == 0 )
		{
			ft_nanosleep( 500000000 );
			continue;
		}

		while ( !ft_mutex_try_lock( &loader.mutex ) ) {}

		FT_TRACE( "loader jobs count %d", loader.job_count );

		ft_begin_command_buffer( loader.cmd );

		struct ft_loader_job* job = loader.head;

		while ( job )
		{
			complete_job( loader.cmd, job );

			job = job->next;
		}

		ft_end_command_buffer( loader.cmd );
		ft_immediate_submit( loader.queue, loader.cmd );

		job = loader.head;

		loader.head      = NULL;
		loader.tail      = NULL;
		loader.job_count = 0;

		ft_mutex_unlock( &loader.mutex );

		while ( job )
		{
			if ( job->staging_buffer )
			{
				ft_destroy_buffer( loader.device, job->staging_buffer );
			}

			struct ft_loader_job* tmp = job;
			job                       = job->next;
			free( tmp );
		}
	}

	return 0;
}

void
ft_upload_buffer( const struct ft_buffer_upload_job* job )
{
	FT_ASSERT( job );
	FT_ASSERT( job->buffer );
	FT_ASSERT( job->data );

	struct ft_buffer* staging_buffer = create_staging_buffer( job->size );
	ft_map_memory( loader.device, staging_buffer );
	memcpy( staging_buffer->mapped_memory, job->data, job->size );
	ft_unmap_memory( loader.device, staging_buffer );

	add_job( &( struct ft_loader_job ) {
	    .type              = FT_LOADER_JOB_TYPE_BUFFER_UPLOAD,
	    .buffer_upload_job = *job,
	    .staging_buffer    = staging_buffer,
	} );
}

void
ft_upload_image( const struct ft_image_upload_job* job )
{
	FT_ASSERT( job );
	FT_ASSERT( job->image );
	FT_ASSERT( job->data );

	uint64_t upload_size =
	    job->width * job->height * ft_format_size_bytes( job->image->format );

	struct ft_buffer* staging_buffer = create_staging_buffer( upload_size );
	ft_map_memory( loader.device, staging_buffer );
	memcpy( staging_buffer->mapped_memory, job->data, upload_size );
	ft_unmap_memory( loader.device, staging_buffer );

	add_job( &( struct ft_loader_job ) {
	    .type             = FT_LOADER_JOB_TYPE_IMAGE_UPLOAD,
	    .image_upload_job = *job,
	    .staging_buffer   = staging_buffer,
	} );
}

void
ft_generate_mipmaps( const struct ft_generate_mipmaps_job* job )
{
	add_job( &( struct ft_loader_job ) {
	    .type                 = FT_LOADER_JOB_TYPE_GENERATE_MIPMAPS,
	    .generate_mipmaps_job = *job,
	} );
}

void
ft_resource_loader_wait_idle()
{
	while ( loader.job_count != 0 ) {}
}