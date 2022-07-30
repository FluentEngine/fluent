#include "renderer_private.h"
#include "renderer_enums_stringifier.h"
#include "vulkan/vulkan_backend.h"
#include "d3d12/d3d12_backend.h"
#include "metal/metal_backend.h"
#include "renderer_backend.h"

ft_destroy_renderer_backend_fun      ft_destroy_renderer_backend_impl;
ft_create_device_fun                 ft_create_device_impl;
ft_destroy_device_fun                ft_destroy_device_impl;
ft_create_queue_fun                  ft_create_queue_impl;
ft_destroy_queue_fun                 ft_destroy_queue_impl;
ft_queue_wait_idle_fun               ft_queue_wait_idle_impl;
ft_queue_submit_fun                  ft_queue_submit_impl;
ft_immediate_submit_fun              ft_immediate_submit_impl;
ft_queue_present_fun                 ft_queue_present_impl;
ft_create_semaphore_fun              ft_create_semaphore_impl;
ft_destroy_semaphore_fun             ft_destroy_semaphore_impl;
ft_create_fence_fun                  ft_create_fence_impl;
ft_destroy_fence_fun                 ft_destroy_fence_impl;
ft_wait_for_fences_fun               ft_wait_for_fences_impl;
ft_reset_fences_fun                  ft_reset_fences_impl;
ft_create_swapchain_fun              ft_create_swapchain_impl;
ft_resize_swapchain_fun              ft_resize_swapchain_impl;
ft_destroy_swapchain_fun             ft_destroy_swapchain_impl;
ft_create_command_pool_fun           ft_create_command_pool_impl;
ft_destroy_command_pool_fun          ft_destroy_command_pool_impl;
ft_create_command_buffers_fun        ft_create_command_buffers_impl;
ft_free_command_buffers_fun          ft_free_command_buffers_impl;
ft_destroy_command_buffers_fun       ft_destroy_command_buffers_impl;
ft_begin_command_buffer_fun          ft_begin_command_buffer_impl;
ft_end_command_buffer_fun            ft_end_command_buffer_impl;
ft_acquire_next_image_fun            ft_acquire_next_image_impl;
ft_create_shader_fun                 ft_create_shader_impl;
ft_destroy_shader_fun                ft_destroy_shader_impl;
ft_create_descriptor_set_layout_fun  ft_create_descriptor_set_layout_impl;
ft_destroy_descriptor_set_layout_fun ft_destroy_descriptor_set_layout_impl;
ft_create_pipeline_fun               ft_create_pipeline_impl;
ft_destroy_pipeline_fun              ft_destroy_pipeline_impl;
ft_create_buffer_fun                 ft_create_buffer_impl;
ft_destroy_buffer_fun                ft_destroy_buffer_impl;
ft_map_memory_fun                    ft_map_memory_impl;
ft_unmap_memory_fun                  ft_unmap_memory_impl;
ft_create_sampler_fun                ft_create_sampler_impl;
ft_destroy_sampler_fun               ft_destroy_sampler_impl;
ft_create_image_fun                  ft_create_image_impl;
ft_destroy_image_fun                 ft_destroy_image_impl;
ft_create_descriptor_set_fun         ft_create_descriptor_set_impl;
ft_destroy_descriptor_set_fun        ft_destroy_descriptor_set_impl;
ft_update_descriptor_set_fun         ft_update_descriptor_set_impl;
ft_cmd_begin_render_pass_fun         ft_cmd_begin_render_pass_impl;
ft_cmd_end_render_pass_fun           ft_cmd_end_render_pass_impl;
ft_cmd_barrier_fun                   ft_cmd_barrier_impl;
ft_cmd_set_scissor_fun               ft_cmd_set_scissor_impl;
ft_cmd_set_viewport_fun              ft_cmd_set_viewport_impl;
ft_cmd_bind_pipeline_fun             ft_cmd_bind_pipeline_impl;
ft_cmd_draw_fun                      ft_cmd_draw_impl;
ft_cmd_draw_indexed_fun              ft_cmd_draw_indexed_impl;
ft_cmd_bind_vertex_buffer_fun        ft_cmd_bind_vertex_buffer_impl;
ft_cmd_bind_index_buffer_fun         ft_cmd_bind_index_buffer_impl;
ft_cmd_copy_buffer_fun               ft_cmd_copy_buffer_impl;
ft_cmd_copy_buffer_to_image_fun      ft_cmd_copy_buffer_to_image_impl;
ft_cmd_bind_descriptor_set_fun       ft_cmd_bind_descriptor_set_impl;
ft_cmd_dispatch_fun                  ft_cmd_dispatch_impl;
ft_cmd_push_constants_fun            ft_cmd_push_constants_impl;
ft_cmd_draw_indexed_indirect_fun     ft_cmd_draw_indexed_indirect_impl;
ft_cmd_begin_debug_marker_fun        ft_cmd_begin_debug_marker_impl;
ft_cmd_end_debug_marker_fun          ft_cmd_end_debug_marker_impl;

void
ft_create_renderer_backend( const struct ft_renderer_backend_info* info,
                            struct ft_renderer_backend**           p )
{
	FT_ASSERT( info );
	FT_ASSERT( info->wsi_info );
	FT_ASSERT( p );

	switch ( info->api )
	{
#if FT_VULKAN_BACKEND
	case FT_RENDERER_API_VULKAN:
	{
		vk_create_renderer_backend( info, p );
		break;
	}
#endif
#ifdef D3D12_BACKEND
	case FT_RENDERER_API_D3D12:
	{
		d3d12_create_renderer_backend( info, p );
		break;
	}
#endif
#ifdef METAL_BACKEND
	case FT_RENDERER_API_METAL:
	{
		mtl_create_renderer_backend( info, p );
		break;
	}
#endif
	default: FT_ASSERT( 0 && "no supported api available" );
	}

	struct ft_renderer_backend* backend = *p;
	backend->api                        = info->api;

	FT_INFO( "created renderer backend"
	         "\n\t %s",
	         ft_renderer_api_to_string( backend->api ) );
}

void
ft_destroy_renderer_backend( struct ft_renderer_backend* backend )
{
	FT_ASSERT( backend );

	ft_destroy_renderer_backend_impl( backend );
}

void
ft_create_device( const struct ft_renderer_backend* backend,
                  const struct ft_device_info*      info,
                  struct ft_device**                p )
{
	FT_ASSERT( backend );
	FT_ASSERT( info );
	FT_ASSERT( info->backend );
	FT_ASSERT( p );

	ft_create_device_impl( backend, info, p );
	struct ft_device* device = *p;
	device->api              = backend->api;
}

void
ft_destroy_device( struct ft_device* device )
{
	FT_ASSERT( device );

	ft_destroy_device_impl( device );
}

void
ft_create_queue( const struct ft_device*     device,
                 const struct ft_queue_info* info,
                 struct ft_queue**           queue )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( queue );

	ft_create_queue_impl( device, info, queue );
}

void
ft_destroy_queue( struct ft_queue* queue )
{
	FT_ASSERT( queue );

	ft_destroy_queue_impl( queue );
}

void
ft_queue_wait_idle( const struct ft_queue* queue )
{
	FT_ASSERT( queue );

	ft_queue_wait_idle_impl( queue );
}

void
ft_queue_submit( const struct ft_queue*             queue,
                 const struct ft_queue_submit_info* info )
{
	FT_ASSERT( queue );
	FT_ASSERT( info );
	FT_ASSERT( info->command_buffer_count );
	FT_ASSERT( info->command_buffers );

	ft_queue_submit_impl( queue, info );
}

void
ft_immediate_submit( const struct ft_queue*    queue,
                     struct ft_command_buffer* cmd )
{
	FT_ASSERT( queue );
	FT_ASSERT( cmd );

	ft_immediate_submit_impl( queue, cmd );
}

void
ft_queue_present( const struct ft_queue*              queue,
                  const struct ft_queue_present_info* info )
{
	FT_ASSERT( queue );
	FT_ASSERT( info );
	FT_ASSERT( info->swapchain );

	ft_queue_present_impl( queue, info );
}

void
ft_create_semaphore( const struct ft_device* device,
                     struct ft_semaphore**   semaphore )
{
	FT_ASSERT( device );
	FT_ASSERT( semaphore );

	ft_create_semaphore_impl( device, semaphore );
}

void
ft_destroy_semaphore( const struct ft_device* device,
                      struct ft_semaphore*    semaphore )
{
	FT_ASSERT( device );
	FT_ASSERT( semaphore );

	ft_destroy_semaphore_impl( device, semaphore );
}

void
ft_create_fence( const struct ft_device* device, struct ft_fence** fence )
{
	FT_ASSERT( device );
	FT_ASSERT( fence );

	ft_create_fence_impl( device, fence );
}

void
ft_destroy_fence( const struct ft_device* device, struct ft_fence* fence )
{
	FT_ASSERT( device );
	FT_ASSERT( fence );

	ft_destroy_fence_impl( device, fence );
}

void
ft_wait_for_fences( const struct ft_device* device,
                    uint32_t                count,
                    struct ft_fence**       fences )
{
	FT_ASSERT( device );
	FT_ASSERT( count );
	FT_ASSERT( fences );

	ft_wait_for_fences_impl( device, count, fences );
}

void
ft_reset_fences( const struct ft_device* device,
                 uint32_t                count,
                 struct ft_fence**       fences )
{
	FT_ASSERT( device );
	FT_ASSERT( count );
	FT_ASSERT( fences );

	ft_reset_fences_impl( device, count, fences );
}

void
ft_create_swapchain( const struct ft_device*         device,
                     const struct ft_swapchain_info* info,
                     struct ft_swapchain**           p )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( info->queue );
	FT_ASSERT( info->width > 0 );
	FT_ASSERT( info->height > 0 );
	FT_ASSERT( info->wsi_info );
	FT_ASSERT( p );

	ft_create_swapchain_impl( device, info, p );

	struct ft_swapchain* swapchain = *p;
	FT_UNUSED( swapchain );

	FT_INFO( "created swapchain"
	         "\n\t width: %d height: %d"
	         "\n\t image count: %d"
	         "\n\t format: %s"
	         "\n\t vsync: %s",
	         swapchain->width,
	         swapchain->height,
	         swapchain->image_count,
	         ft_format_to_string( swapchain->format ),
	         swapchain->vsync ? "enabled" : "disabled" );
}

void
ft_resize_swapchain( const struct ft_device* device,
                     struct ft_swapchain*    swapchain,
                     uint32_t                width,
                     uint32_t                height )
{
	FT_ASSERT( device );
	FT_ASSERT( swapchain );
	FT_ASSERT( width > 0 );
	FT_ASSERT( height > 0 );

	ft_resize_swapchain_impl( device, swapchain, width, height );

	FT_INFO( "resized swapchain"
	         "\n\t width: %d height: %d"
	         "\n\t image count: %d"
	         "\n\t vsync: %s",
	         swapchain->width,
	         swapchain->height,
	         swapchain->image_count,
	         swapchain->vsync ? "enabled" : "disabled" );
}

void
ft_destroy_swapchain( const struct ft_device* device,
                      struct ft_swapchain*    swapchain )
{
	FT_ASSERT( device );
	FT_ASSERT( swapchain );

	ft_destroy_swapchain_impl( device, swapchain );
}

void
ft_create_command_pool( const struct ft_device*            device,
                        const struct ft_command_pool_info* info,
                        struct ft_command_pool**           command_pool )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( info->queue );
	FT_ASSERT( command_pool );

	ft_create_command_pool_impl( device, info, command_pool );
}

void
ft_destroy_command_pool( const struct ft_device* device,
                         struct ft_command_pool* command_pool )
{
	FT_ASSERT( device );
	FT_ASSERT( command_pool );

	ft_destroy_command_pool_impl( device, command_pool );
}

void
ft_create_command_buffers( const struct ft_device*       device,
                           const struct ft_command_pool* command_pool,
                           uint32_t                      count,
                           struct ft_command_buffer**    command_buffers )
{
	FT_ASSERT( device );
	FT_ASSERT( command_pool );
	FT_ASSERT( count > 0 );
	FT_ASSERT( command_buffers );

	ft_create_command_buffers_impl( device,
	                                command_pool,
	                                count,
	                                command_buffers );
}

void
ft_free_command_buffers( const struct ft_device*       device,
                         const struct ft_command_pool* command_pool,
                         uint32_t                      count,
                         struct ft_command_buffer**    command_buffers )
{
	FT_ASSERT( device );
	FT_ASSERT( command_pool );
	FT_ASSERT( count > 0 );
	FT_ASSERT( command_buffers );

	ft_free_command_buffers_impl( device,
	                              command_pool,
	                              count,
	                              command_buffers );
}

void
ft_destroy_command_buffers( const struct ft_device*       device,
                            const struct ft_command_pool* command_pool,
                            uint32_t                      count,
                            struct ft_command_buffer**    command_buffers )
{
	FT_ASSERT( device );
	FT_ASSERT( command_pool );
	FT_ASSERT( count > 0 );
	FT_ASSERT( command_buffers );

	ft_destroy_command_buffers_impl( device,
	                                 command_pool,
	                                 count,
	                                 command_buffers );
}

void
ft_begin_command_buffer( const struct ft_command_buffer* cmd )
{
	FT_ASSERT( cmd );

	ft_begin_command_buffer_impl( cmd );
}

void
ft_end_command_buffer( const struct ft_command_buffer* cmd )
{
	FT_ASSERT( cmd );

	ft_end_command_buffer_impl( cmd );
}

void
ft_acquire_next_image( const struct ft_device*    device,
                       const struct ft_swapchain* swapchain,
                       const struct ft_semaphore* semaphore,
                       const struct ft_fence*     fence,
                       uint32_t*                  image_index )
{
	FT_ASSERT( device );
	FT_ASSERT( swapchain );

	ft_acquire_next_image_impl( device,
	                            swapchain,
	                            semaphore,
	                            fence,
	                            image_index );
}

void
ft_create_shader( const struct ft_device* device,
                  struct ft_shader_info*  info,
                  struct ft_shader**      shader )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( shader );

	ft_create_shader_impl( device, info, shader );
}

void
ft_destroy_shader( const struct ft_device* device, struct ft_shader* shader )
{
	FT_ASSERT( device );
	FT_ASSERT( shader );

	ft_destroy_shader_impl( device, shader );
}

void
ft_create_descriptor_set_layout(
    const struct ft_device*           device,
    struct ft_shader*                 shader,
    struct ft_descriptor_set_layout** descriptor_set_layout )
{
	FT_ASSERT( device );
	FT_ASSERT( shader );
	FT_ASSERT( descriptor_set_layout );

	ft_create_descriptor_set_layout_impl( device,
	                                      shader,
	                                      descriptor_set_layout );
}

void
ft_destroy_descriptor_set_layout( const struct ft_device*          device,
                                  struct ft_descriptor_set_layout* layout )
{
	FT_ASSERT( device );
	FT_ASSERT( layout );

	ft_destroy_descriptor_set_layout_impl( device, layout );
}

void
ft_create_pipeline( const struct ft_device*        device,
                    const struct ft_pipeline_info* info,
                    struct ft_pipeline**           pipeline )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( pipeline );

	ft_create_pipeline_impl( device, info, pipeline );
}

void
ft_destroy_pipeline( const struct ft_device* device,
                     struct ft_pipeline*     pipeline )
{
	FT_ASSERT( device );
	FT_ASSERT( pipeline );

	ft_destroy_pipeline_impl( device, pipeline );
}

void
ft_cmd_begin_render_pass( const struct ft_command_buffer*         cmd,
                          const struct ft_render_pass_begin_info* info )
{
	FT_ASSERT( cmd );
	FT_ASSERT( info );
	FT_ASSERT( info->width > 0 );
	FT_ASSERT( info->height > 0 );

	ft_cmd_begin_render_pass_impl( cmd, info );
}

void
ft_cmd_end_render_pass( const struct ft_command_buffer* cmd )
{
	FT_ASSERT( cmd );

	ft_cmd_end_render_pass_impl( cmd );
}

void
ft_cmd_barrier( const struct ft_command_buffer* cmd,
                uint32_t                        memory_barriers_count,
                const struct ft_memory_barrier* memory_barrier,
                uint32_t                        buffer_barriers_count,
                const struct ft_buffer_barrier* buffer_barriers,
                uint32_t                        image_barriers_count,
                const struct ft_image_barrier*  image_barriers )
{
	FT_ASSERT( cmd );

	ft_cmd_barrier_impl( cmd,
	                     memory_barriers_count,
	                     memory_barrier,
	                     buffer_barriers_count,
	                     buffer_barriers,
	                     image_barriers_count,
	                     image_barriers );
}

void
ft_cmd_set_scissor( const struct ft_command_buffer* cmd,
                    int32_t                         x,
                    int32_t                         y,
                    uint32_t                        width,
                    uint32_t                        height )
{
	FT_ASSERT( cmd );

	ft_cmd_set_scissor_impl( cmd, x, y, width, height );
}

void
ft_cmd_set_viewport( const struct ft_command_buffer* cmd,
                     float                           x,
                     float                           y,
                     float                           width,
                     float                           height,
                     float                           min_depth,
                     float                           max_depth )
{
	FT_ASSERT( cmd );

	ft_cmd_set_viewport_impl( cmd, x, y, width, height, min_depth, max_depth );
}

void
ft_cmd_bind_pipeline( const struct ft_command_buffer* cmd,
                      const struct ft_pipeline*       pipeline )
{
	FT_ASSERT( cmd );
	FT_ASSERT( pipeline );

	ft_cmd_bind_pipeline_impl( cmd, pipeline );
}

void
ft_cmd_draw( const struct ft_command_buffer* cmd,
             uint32_t                        vertex_count,
             uint32_t                        instance_count,
             uint32_t                        first_vertex,
             uint32_t                        first_instance )
{
	FT_ASSERT( cmd );

	ft_cmd_draw_impl( cmd,
	                  vertex_count,
	                  instance_count,
	                  first_vertex,
	                  first_instance );
}

void
ft_cmd_draw_indexed( const struct ft_command_buffer* cmd,
                     uint32_t                        index_count,
                     uint32_t                        instance_count,
                     uint32_t                        first_index,
                     int32_t                         vertex_offset,
                     uint32_t                        first_instance )
{
	FT_ASSERT( cmd );

	ft_cmd_draw_indexed_impl( cmd,
	                          index_count,
	                          instance_count,
	                          first_index,
	                          vertex_offset,
	                          first_instance );
}

void
ft_cmd_bind_vertex_buffer( const struct ft_command_buffer* cmd,
                           const struct ft_buffer*         buffer,
                           const uint64_t                  offset )
{
	FT_ASSERT( cmd );
	FT_ASSERT( buffer );
	FT_ASSERT( offset < buffer->size );

	ft_cmd_bind_vertex_buffer_impl( cmd, buffer, offset );
}

void
ft_cmd_bind_index_buffer( const struct ft_command_buffer* cmd,
                          const struct ft_buffer*         buffer,
                          const uint64_t                  offset,
                          enum ft_index_type              index_type )
{
	FT_ASSERT( cmd );
	FT_ASSERT( buffer );
	FT_ASSERT( offset <= buffer->size );

	ft_cmd_bind_index_buffer_impl( cmd, buffer, offset, index_type );
}

void
ft_cmd_copy_buffer( const struct ft_command_buffer* cmd,
                    const struct ft_buffer*         src,
                    uint64_t                        src_offset,
                    struct ft_buffer*               dst,
                    uint64_t                        dst_offset,
                    uint64_t                        size )
{
	FT_ASSERT( cmd );
	FT_ASSERT( src );
	FT_ASSERT( dst );
	FT_ASSERT( dst_offset + size <= dst->size );

	ft_cmd_copy_buffer_impl( cmd, src, src_offset, dst, dst_offset, size );
}

void
ft_cmd_copy_buffer_to_image( const struct ft_command_buffer*    cmd,
                             const struct ft_buffer*            src,
                             struct ft_image*                   dst,
                             const struct ft_buffer_image_copy* copy )
{
	FT_ASSERT( cmd );
	FT_ASSERT( src );
	FT_ASSERT( dst );
	FT_ASSERT( copy );

	ft_cmd_copy_buffer_to_image_impl( cmd, src, dst, copy );
}

void
ft_cmd_bind_descriptor_set( const struct ft_command_buffer* cmd,
                            uint32_t                        first_set,
                            const struct ft_descriptor_set* set,
                            const struct ft_pipeline*       pipeline )
{
	FT_ASSERT( cmd );
	FT_ASSERT( set );
	FT_ASSERT( pipeline );

	ft_cmd_bind_descriptor_set_impl( cmd, first_set, set, pipeline );
}

void
ft_cmd_dispatch( const struct ft_command_buffer* cmd,
                 uint32_t                        group_count_x,
                 uint32_t                        group_count_y,
                 uint32_t                        group_count_z )
{
	FT_ASSERT( cmd );
	FT_ASSERT( group_count_x );
	FT_ASSERT( group_count_y );
	FT_ASSERT( group_count_z );

	ft_cmd_dispatch_impl( cmd, group_count_x, group_count_y, group_count_z );
}

void
ft_cmd_push_constants( const struct ft_command_buffer* cmd,
                       const struct ft_pipeline*       pipeline,
                       uint32_t                        offset,
                       uint32_t                        size,
                       const void*                     data )
{
	FT_ASSERT( cmd );
	FT_ASSERT( pipeline );
	FT_ASSERT( data );

	ft_cmd_push_constants_impl( cmd, pipeline, offset, size, data );
}

void
ft_create_buffer( const struct ft_device*      device,
                  const struct ft_buffer_info* info,
                  struct ft_buffer**           buffer )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( info->size );
	FT_ASSERT( buffer );

	ft_create_buffer_impl( device, info, buffer );
}

void
ft_destroy_buffer( const struct ft_device* device, struct ft_buffer* buffer )
{
	FT_ASSERT( device );
	FT_ASSERT( buffer );

	ft_destroy_buffer_impl( device, buffer );
}

void*
ft_map_memory( const struct ft_device* device, struct ft_buffer* buffer )
{
	FT_ASSERT( device );
	FT_ASSERT( buffer );
	FT_ASSERT( buffer->mapped_memory == NULL );

	return ft_map_memory_impl( device, buffer );
}

void
ft_unmap_memory( const struct ft_device* device, struct ft_buffer* buffer )
{
	FT_ASSERT( device );
	FT_ASSERT( buffer );
	FT_ASSERT( buffer->mapped_memory );

	ft_unmap_memory_impl( device, buffer );
}

void
ft_cmd_draw_indexed_indirect( const struct ft_command_buffer* cmd,
                              const struct ft_buffer*         buffer,
                              uint64_t                        offset,
                              uint32_t                        draw_count,
                              uint32_t                        stride )
{
	FT_ASSERT( cmd );
	FT_ASSERT( buffer );

	ft_cmd_draw_indexed_indirect_impl( cmd,
	                                   buffer,
	                                   offset,
	                                   draw_count,
	                                   stride );
}

void
ft_cmd_begin_debug_marker( const struct ft_command_buffer* cmd,
                           const char*                     name,
                           float                           color[ 4 ] )
{
	FT_ASSERT( cmd );

	ft_cmd_begin_debug_marker_impl( cmd, name, color );
}

void
ft_cmd_end_debug_marker( const struct ft_command_buffer* cmd )
{
	FT_ASSERT( cmd );

	ft_cmd_end_debug_marker_impl( cmd );
}

void
ft_create_sampler( const struct ft_device*       device,
                   const struct ft_sampler_info* info,
                   struct ft_sampler**           sampler )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( sampler );

	ft_create_sampler_impl( device, info, sampler );
}

void
ft_destroy_sampler( const struct ft_device* device, struct ft_sampler* sampler )
{
	FT_ASSERT( device );
	FT_ASSERT( sampler );

	ft_destroy_sampler_impl( device, sampler );
}

void
ft_create_image( const struct ft_device*     device,
                 const struct ft_image_info* info,
                 struct ft_image**           image )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( info->width > 0 );
	FT_ASSERT( info->height > 0 );
	FT_ASSERT( info->layer_count > 0 );
	FT_ASSERT( info->mip_levels > 0 );
	FT_ASSERT( info->sample_count > 0 );
	FT_ASSERT( image );

	ft_create_image_impl( device, info, image );

	( *image )->width           = info->width;
	( *image )->height          = info->height;
	( *image )->depth           = info->depth;
	( *image )->format          = info->format;
	( *image )->sample_count    = info->sample_count;
	( *image )->mip_levels      = info->mip_levels;
	( *image )->layer_count     = info->layer_count;
	( *image )->descriptor_type = info->descriptor_type;
}

void
ft_destroy_image( const struct ft_device* device, struct ft_image* image )
{
	FT_ASSERT( device );
	FT_ASSERT( image );

	ft_destroy_image_impl( device, image );
}

void
ft_create_descriptor_set( const struct ft_device*              device,
                          const struct ft_descriptor_set_info* info,
                          struct ft_descriptor_set**           descriptor_set )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( info->descriptor_set_layout );
	FT_ASSERT( descriptor_set );

	ft_create_descriptor_set_impl( device, info, descriptor_set );
}

void
ft_destroy_descriptor_set( const struct ft_device*   device,
                           struct ft_descriptor_set* set )
{
	FT_ASSERT( device );
	FT_ASSERT( set );

	ft_destroy_descriptor_set_impl( device, set );
}

void
ft_update_descriptor_set( const struct ft_device*           device,
                          struct ft_descriptor_set*         set,
                          uint32_t                          count,
                          const struct ft_descriptor_write* writes )
{
	FT_ASSERT( device );
	FT_ASSERT( set );
	FT_ASSERT( count );
	FT_ASSERT( writes );

	ft_update_descriptor_set_impl( device, set, count, writes );
}

enum ft_renderer_api
ft_get_device_api( const struct ft_device* device )
{
	return device->api;
}

void
ft_get_swapchain_size( const struct ft_swapchain* swapchain,
                       uint32_t*                  width,
                       uint32_t*                  height )
{
	*width  = swapchain->width;
	*height = swapchain->height;
}

enum ft_format
ft_get_swapchain_format( const struct ft_swapchain* swapchain )
{
	return swapchain->format;
}

struct ft_image*
ft_get_swapchain_image( const struct ft_swapchain* swapchain, uint32_t index )
{
	FT_ASSERT( index < swapchain->image_count );
	return swapchain->images[ index ];
}
