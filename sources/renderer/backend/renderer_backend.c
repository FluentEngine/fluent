#include "renderer_private.h"
#include "renderer_backend.h"
#include "vulkan/vulkan_backend.h"
#include "d3d12/d3d12_backend.h"
#include "metal/metal_backend.h"

destroy_renderer_backend_fun      destroy_renderer_backend_impl;
create_device_fun                 create_device_impl;
destroy_device_fun                destroy_device_impl;
create_queue_fun                  create_queue_impl;
destroy_queue_fun                 destroy_queue_impl;
queue_wait_idle_fun               queue_wait_idle_impl;
queue_submit_fun                  queue_submit_impl;
immediate_submit_fun              immediate_submit_impl;
queue_present_fun                 queue_present_impl;
create_semaphore_fun              create_semaphore_impl;
destroy_semaphore_fun             destroy_semaphore_impl;
create_fence_fun                  create_fence_impl;
destroy_fence_fun                 destroy_fence_impl;
wait_for_fences_fun               wait_for_fences_impl;
reset_fences_fun                  reset_fences_impl;
create_swapchain_fun              create_swapchain_impl;
resize_swapchain_fun              resize_swapchain_impl;
destroy_swapchain_fun             destroy_swapchain_impl;
create_command_pool_fun           create_command_pool_impl;
destroy_command_pool_fun          destroy_command_pool_impl;
create_command_buffers_fun        create_command_buffers_impl;
free_command_buffers_fun          free_command_buffers_impl;
destroy_command_buffers_fun       destroy_command_buffers_impl;
begin_command_buffer_fun          begin_command_buffer_impl;
end_command_buffer_fun            end_command_buffer_impl;
acquire_next_image_fun            acquire_next_image_impl;
create_shader_fun                 create_shader_impl;
destroy_shader_fun                destroy_shader_impl;
create_descriptor_set_layout_fun  create_descriptor_set_layout_impl;
destroy_descriptor_set_layout_fun destroy_descriptor_set_layout_impl;
create_compute_pipeline_fun       create_compute_pipeline_impl;
create_graphics_pipeline_fun      create_graphics_pipeline_impl;
destroy_pipeline_fun              destroy_pipeline_impl;
create_buffer_fun                 create_buffer_impl;
destroy_buffer_fun                destroy_buffer_impl;
map_memory_fun                    map_memory_impl;
unmap_memory_fun                  unmap_memory_impl;
create_sampler_fun                create_sampler_impl;
destroy_sampler_fun               destroy_sampler_impl;
create_image_fun                  create_image_impl;
destroy_image_fun                 destroy_image_impl;
create_descriptor_set_fun         create_descriptor_set_impl;
destroy_descriptor_set_fun        destroy_descriptor_set_impl;
update_descriptor_set_fun         update_descriptor_set_impl;
cmd_begin_render_pass_fun         cmd_begin_render_pass_impl;
cmd_end_render_pass_fun           cmd_end_render_pass_impl;
cmd_barrier_fun                   cmd_barrier_impl;
cmd_set_scissor_fun               cmd_set_scissor_impl;
cmd_set_viewport_fun              cmd_set_viewport_impl;
cmd_bind_pipeline_fun             cmd_bind_pipeline_impl;
cmd_draw_fun                      cmd_draw_impl;
cmd_draw_indexed_fun              cmd_draw_indexed_impl;
cmd_bind_vertex_buffer_fun        cmd_bind_vertex_buffer_impl;
cmd_bind_index_buffer_u16_fun     cmd_bind_index_buffer_u16_impl;
cmd_bind_index_buffer_u32_fun     cmd_bind_index_buffer_u32_impl;
cmd_copy_buffer_fun               cmd_copy_buffer_impl;
cmd_copy_buffer_to_image_fun      cmd_copy_buffer_to_image_impl;
cmd_bind_descriptor_set_fun       cmd_bind_descriptor_set_impl;
cmd_dispatch_fun                  cmd_dispatch_impl;
cmd_push_constants_fun            cmd_push_constants_impl;
cmd_clear_color_image_fun         cmd_clear_color_image_impl;
cmd_draw_indexed_indirect_fun     cmd_draw_indexed_indirect_impl;

// defined in resource_loader.c
void
resource_loader_init( const struct Device* device, u64 staging_buffer_size );

void
resource_loader_shutdown();

void
create_renderer_backend( const struct RendererBackendInfo* info,
                         struct RendererBackend**          p )
{
	FT_ASSERT( info );
	FT_ASSERT( info->wsi_info );
	FT_ASSERT( p );

	switch ( info->api )
	{
#ifdef VULKAN_BACKEND
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

	struct RendererBackend* backend = *p;
	backend->api                    = info->api;
}

void
destroy_renderer_backend( struct RendererBackend* backend )
{
	FT_ASSERT( backend );

	destroy_renderer_backend_impl( backend );
}

void
create_device( const struct RendererBackend* backend,
               const struct DeviceInfo*      info,
               struct Device**               p )
{
	FT_ASSERT( backend );
	FT_ASSERT( info );
	FT_ASSERT( info->backend );
	FT_ASSERT( p );

	create_device_impl( backend, info, p );
	struct Device* device = *p;
	device->api           = backend->api;
	resource_loader_init( device, RESOURCE_LOADER_STAGING_BUFFER_SIZE );
}

void
destroy_device( struct Device* device )
{
	FT_ASSERT( device );

	resource_loader_shutdown();
	destroy_device_impl( device );
}

void
create_queue( const struct Device*    device,
              const struct QueueInfo* info,
              struct Queue**          queue )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( queue );

	create_queue_impl( device, info, queue );
}

void
destroy_queue( struct Queue* queue )
{
	FT_ASSERT( queue );

	destroy_queue_impl( queue );
}

void
queue_wait_idle( const struct Queue* queue )
{
	FT_ASSERT( queue );

	queue_wait_idle_impl( queue );
}

void
queue_submit( const struct Queue* queue, const struct QueueSubmitInfo* info )
{
	FT_ASSERT( queue );
	FT_ASSERT( info );
	FT_ASSERT( info->command_buffer_count );
	FT_ASSERT( info->command_buffers );

	queue_submit_impl( queue, info );
}

void
immediate_submit( const struct Queue* queue, struct CommandBuffer* cmd )
{
	FT_ASSERT( queue );
	FT_ASSERT( cmd );

	immediate_submit_impl( queue, cmd );
}

void
queue_present( const struct Queue* queue, const struct QueuePresentInfo* info )
{
	FT_ASSERT( queue );
	FT_ASSERT( info );
	FT_ASSERT( info->swapchain );

	queue_present_impl( queue, info );
}

void
create_semaphore( const struct Device* device, struct Semaphore** semaphore )
{
	FT_ASSERT( device );
	FT_ASSERT( semaphore );

	create_semaphore_impl( device, semaphore );
}

void
destroy_semaphore( const struct Device* device, struct Semaphore* semaphore )
{
	FT_ASSERT( device );
	FT_ASSERT( semaphore );

	destroy_semaphore_impl( device, semaphore );
}

void
create_fence( const struct Device* device, struct Fence** fence )
{
	FT_ASSERT( device );
	FT_ASSERT( fence );

	create_fence_impl( device, fence );
}

void
destroy_fence( const struct Device* device, struct Fence* fence )
{
	FT_ASSERT( device );
	FT_ASSERT( fence );

	destroy_fence_impl( device, fence );
}

void
wait_for_fences( const struct Device* device, u32 count, struct Fence** fences )
{
	FT_ASSERT( device );
	FT_ASSERT( count );
	FT_ASSERT( fences );

	wait_for_fences_impl( device, count, fences );
}

void
reset_fences( const struct Device* device, u32 count, struct Fence** fences )
{
	FT_ASSERT( device );
	FT_ASSERT( count );
	FT_ASSERT( fences );

	reset_fences_impl( device, count, fences );
}

void
create_swapchain( const struct Device*        device,
                  const struct SwapchainInfo* info,
                  struct Swapchain**          swapchain )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( info->queue );
	FT_ASSERT( info->width > 0 );
	FT_ASSERT( info->height > 0 );
	FT_ASSERT( info->wsi_info );
	FT_ASSERT( swapchain );

	create_swapchain_impl( device, info, swapchain );
}

void
resize_swapchain( const struct Device* device,
                  struct Swapchain*    swapchain,
                  u32                  width,
                  u32                  height )
{
	FT_ASSERT( device );
	FT_ASSERT( swapchain );
	FT_ASSERT( width > 0 );
	FT_ASSERT( height > 0 );

	resize_swapchain_impl( device, swapchain, width, height );
}

void
destroy_swapchain( const struct Device* device, struct Swapchain* swapchain )
{
	FT_ASSERT( device );
	FT_ASSERT( swapchain );

	destroy_swapchain_impl( device, swapchain );
}

void
create_command_pool( const struct Device*          device,
                     const struct CommandPoolInfo* info,
                     struct CommandPool**          command_pool )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( info->queue );
	FT_ASSERT( command_pool );

	create_command_pool_impl( device, info, command_pool );
}

void
destroy_command_pool( const struct Device* device,
                      struct CommandPool*  command_pool )
{
	FT_ASSERT( device );
	FT_ASSERT( command_pool );

	destroy_command_pool_impl( device, command_pool );
}

void
create_command_buffers( const struct Device*      device,
                        const struct CommandPool* command_pool,
                        u32                       count,
                        struct CommandBuffer**    command_buffers )
{
	FT_ASSERT( device );
	FT_ASSERT( command_pool );
	FT_ASSERT( count > 0 );
	FT_ASSERT( command_buffers );

	create_command_buffers_impl( device, command_pool, count, command_buffers );
}

void
free_command_buffers( const struct Device*      device,
                      const struct CommandPool* command_pool,
                      u32                       count,
                      struct CommandBuffer**    command_buffers )
{
	FT_ASSERT( device );
	FT_ASSERT( command_pool );
	FT_ASSERT( count > 0 );
	FT_ASSERT( command_buffers );

	free_command_buffers_impl( device, command_pool, count, command_buffers );
}

void
destroy_command_buffers( const struct Device*      device,
                         const struct CommandPool* command_pool,
                         u32                       count,
                         struct CommandBuffer**    command_buffers )
{
	FT_ASSERT( device );
	FT_ASSERT( command_pool );
	FT_ASSERT( count > 0 );
	FT_ASSERT( command_buffers );

	destroy_command_buffers_impl( device,
	                              command_pool,
	                              count,
	                              command_buffers );
}

void
begin_command_buffer( const struct CommandBuffer* cmd )
{
	FT_ASSERT( cmd );

	begin_command_buffer_impl( cmd );
}

void
end_command_buffer( const struct CommandBuffer* cmd )
{
	FT_ASSERT( cmd );

	end_command_buffer_impl( cmd );
}

void
acquire_next_image( const struct Device*    device,
                    const struct Swapchain* swapchain,
                    const struct Semaphore* semaphore,
                    const struct Fence*     fence,
                    u32*                    image_index )
{
	FT_ASSERT( device );
	FT_ASSERT( swapchain );

	acquire_next_image_impl( device, swapchain, semaphore, fence, image_index );
}

void
create_shader( const struct Device* device,
               struct ShaderInfo*   info,
               struct Shader**      shader )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( shader );

	create_shader_impl( device, info, shader );
}

void
destroy_shader( const struct Device* device, struct Shader* shader )
{
	FT_ASSERT( device );
	FT_ASSERT( shader );

	destroy_shader_impl( device, shader );
}

void
create_descriptor_set_layout(
    const struct Device*         device,
    struct Shader*               shader,
    struct DescriptorSetLayout** descriptor_set_layout )
{
	FT_ASSERT( device );
	FT_ASSERT( shader );
	FT_ASSERT( descriptor_set_layout );

	create_descriptor_set_layout_impl( device, shader, descriptor_set_layout );
}

void
destroy_descriptor_set_layout( const struct Device*        device,
                               struct DescriptorSetLayout* layout )
{
	FT_ASSERT( device );
	FT_ASSERT( layout );

	destroy_descriptor_set_layout_impl( device, layout );
}

void
create_compute_pipeline( const struct Device*       device,
                         const struct PipelineInfo* info,
                         struct Pipeline**          pipeline )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( pipeline );

	create_compute_pipeline_impl( device, info, pipeline );
}

void
create_graphics_pipeline( const struct Device*       device,
                          const struct PipelineInfo* info,
                          struct Pipeline**          pipeline )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( pipeline );

	create_graphics_pipeline_impl( device, info, pipeline );
}

void
destroy_pipeline( const struct Device* device, struct Pipeline* pipeline )
{
	FT_ASSERT( device );
	FT_ASSERT( pipeline );

	destroy_pipeline_impl( device, pipeline );
}

void
cmd_begin_render_pass( const struct CommandBuffer*       cmd,
                       const struct RenderPassBeginInfo* info )
{
	FT_ASSERT( cmd );
	FT_ASSERT( info );
	FT_ASSERT( info->width > 0 );
	FT_ASSERT( info->height > 0 );

	cmd_begin_render_pass_impl( cmd, info );
}

void
cmd_end_render_pass( const struct CommandBuffer* cmd )
{
	FT_ASSERT( cmd );

	cmd_end_render_pass_impl( cmd );
}

void
cmd_barrier( const struct CommandBuffer* cmd,
             u32                         memory_barriers_count,
             const struct MemoryBarrier* memory_barrier,
             u32                         buffer_barriers_count,
             const struct BufferBarrier* buffer_barriers,
             u32                         image_barriers_count,
             const struct ImageBarrier*  image_barriers )
{
	FT_ASSERT( cmd );

	cmd_barrier_impl( cmd,
	                  memory_barriers_count,
	                  memory_barrier,
	                  buffer_barriers_count,
	                  buffer_barriers,
	                  image_barriers_count,
	                  image_barriers );
}

void
cmd_set_scissor( const struct CommandBuffer* cmd,
                 i32                         x,
                 i32                         y,
                 u32                         width,
                 u32                         height )
{
	FT_ASSERT( cmd );

	cmd_set_scissor_impl( cmd, x, y, width, height );
}

void
cmd_set_viewport( const struct CommandBuffer* cmd,
                  f32                         x,
                  f32                         y,
                  f32                         width,
                  f32                         height,
                  f32                         min_depth,
                  f32                         max_depth )
{
	FT_ASSERT( cmd );

	cmd_set_viewport_impl( cmd, x, y, width, height, min_depth, max_depth );
}

void
cmd_bind_pipeline( const struct CommandBuffer* cmd,
                   const struct Pipeline*      pipeline )
{
	FT_ASSERT( cmd );
	FT_ASSERT( pipeline );

	cmd_bind_pipeline_impl( cmd, pipeline );
}

void
cmd_draw( const struct CommandBuffer* cmd,
          u32                         vertex_count,
          u32                         instance_count,
          u32                         first_vertex,
          u32                         first_instance )
{
	FT_ASSERT( cmd );

	cmd_draw_impl( cmd,
	               vertex_count,
	               instance_count,
	               first_vertex,
	               first_instance );
}

void
cmd_draw_indexed( const struct CommandBuffer* cmd,
                  u32                         index_count,
                  u32                         instance_count,
                  u32                         first_index,
                  i32                         vertex_offset,
                  u32                         first_instance )
{
	FT_ASSERT( cmd );

	cmd_draw_indexed_impl( cmd,
	                       index_count,
	                       instance_count,
	                       first_index,
	                       vertex_offset,
	                       first_instance );
}

void
cmd_bind_vertex_buffer( const struct CommandBuffer* cmd,
                        const struct Buffer*        buffer,
                        const u64                   offset )
{
	FT_ASSERT( cmd );
	FT_ASSERT( buffer );
	FT_ASSERT( offset < buffer->size );

	cmd_bind_vertex_buffer_impl( cmd, buffer, offset );
}

void
cmd_bind_index_buffer_u16( const struct CommandBuffer* cmd,
                           const struct Buffer*        buffer,
                           const u64                   offset )
{
	FT_ASSERT( cmd );
	FT_ASSERT( buffer );
	FT_ASSERT( offset < buffer->size );

	cmd_bind_index_buffer_u16_impl( cmd, buffer, offset );
}

void
cmd_bind_index_buffer_u32( const struct CommandBuffer* cmd,
                           const struct Buffer*        buffer,
                           const u64                   offset )
{
	FT_ASSERT( cmd );
	FT_ASSERT( buffer );
	FT_ASSERT( offset <= buffer->size );

	cmd_bind_index_buffer_u32_impl( cmd, buffer, offset );
}

void
cmd_copy_buffer( const struct CommandBuffer* cmd,
                 const struct Buffer*        src,
                 u64                         src_offset,
                 struct Buffer*              dst,
                 u64                         dst_offset,
                 u64                         size )
{
	FT_ASSERT( cmd );
	FT_ASSERT( src );
	FT_ASSERT( dst );
	FT_ASSERT( dst_offset + size <= dst->size );

	cmd_copy_buffer_impl( cmd, src, src_offset, dst, dst_offset, size );
}

void
cmd_copy_buffer_to_image( const struct CommandBuffer* cmd,
                          const struct Buffer*        src,
                          u64                         src_offset,
                          struct Image*               dst )
{
	FT_ASSERT( cmd );
	FT_ASSERT( src );
	FT_ASSERT( dst );

	cmd_copy_buffer_to_image_impl( cmd, src, src_offset, dst );
}

void
cmd_bind_descriptor_set( const struct CommandBuffer* cmd,
                         u32                         first_set,
                         const struct DescriptorSet* set,
                         const struct Pipeline*      pipeline )
{
	FT_ASSERT( cmd );
	FT_ASSERT( set );
	FT_ASSERT( pipeline );

	cmd_bind_descriptor_set_impl( cmd, first_set, set, pipeline );
}

void
cmd_dispatch( const struct CommandBuffer* cmd,
              u32                         group_count_x,
              u32                         group_count_y,
              u32                         group_count_z )
{
	FT_ASSERT( cmd );
	FT_ASSERT( group_count_x );
	FT_ASSERT( group_count_y );
	FT_ASSERT( group_count_z );

	cmd_dispatch_impl( cmd, group_count_x, group_count_y, group_count_z );
}

void
cmd_push_constants( const struct CommandBuffer* cmd,
                    const struct Pipeline*      pipeline,
                    u32                         offset,
                    u32                         size,
                    const void*                 data )
{
	FT_ASSERT( cmd );
	FT_ASSERT( pipeline );
	FT_ASSERT( data );

	cmd_push_constants_impl( cmd, pipeline, offset, size, data );
}

void
cmd_clear_color_image( const struct CommandBuffer* cmd,
                       struct Image*               image,
                       float                       color[ 4 ] )
{
	FT_ASSERT( cmd );
	FT_ASSERT( image );

	cmd_clear_color_image_impl( cmd, image, color );
}

void
create_buffer( const struct Device*     device,
               const struct BufferInfo* info,
               struct Buffer**          buffer )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( info->size );
	FT_ASSERT( buffer );

	create_buffer_impl( device, info, buffer );
}

void
destroy_buffer( const struct Device* device, struct Buffer* buffer )
{
	FT_ASSERT( device );
	FT_ASSERT( buffer );

	destroy_buffer_impl( device, buffer );
}

void*
map_memory( const struct Device* device, struct Buffer* buffer )
{
	FT_ASSERT( device );
	FT_ASSERT( buffer );
	FT_ASSERT( buffer->mapped_memory == NULL );

	return map_memory_impl( device, buffer );
}

void
unmap_memory( const struct Device* device, struct Buffer* buffer )
{
	FT_ASSERT( device );
	FT_ASSERT( buffer );
	FT_ASSERT( buffer->mapped_memory );

	unmap_memory_impl( device, buffer );
}

void
cmd_draw_indexed_indirect( const struct CommandBuffer* cmd,
                           const struct Buffer*        buffer,
                           u64                         offset,
                           u32                         draw_count,
                           u32                         stride )
{
	FT_ASSERT( cmd );
	FT_ASSERT( buffer );

	cmd_draw_indexed_indirect_impl( cmd, buffer, offset, draw_count, stride );
}

void
create_sampler( const struct Device*      device,
                const struct SamplerInfo* info,
                struct Sampler**          sampler )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( sampler );

	create_sampler_impl( device, info, sampler );
}

void
destroy_sampler( const struct Device* device, struct Sampler* sampler )
{
	FT_ASSERT( device );
	FT_ASSERT( sampler );

	destroy_sampler_impl( device, sampler );
}

void
create_image( const struct Device*    device,
              const struct ImageInfo* info,
              struct Image**          image )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( info->width > 0 );
	FT_ASSERT( info->height > 0 );
	FT_ASSERT( info->layer_count > 0 );
	FT_ASSERT( info->mip_levels > 0 );
	FT_ASSERT( info->sample_count > 0 );
	FT_ASSERT( image );

	create_image_impl( device, info, image );
}

void
destroy_image( const struct Device* device, struct Image* image )
{
	FT_ASSERT( device );
	FT_ASSERT( image );

	destroy_image_impl( device, image );
}

void
create_descriptor_set( const struct Device*            device,
                       const struct DescriptorSetInfo* info,
                       struct DescriptorSet**          descriptor_set )
{
	FT_ASSERT( device );
	FT_ASSERT( info );
	FT_ASSERT( info->descriptor_set_layout );
	FT_ASSERT( descriptor_set );

	create_descriptor_set_impl( device, info, descriptor_set );
}

void
destroy_descriptor_set( const struct Device* device, struct DescriptorSet* set )
{
	FT_ASSERT( device );
	FT_ASSERT( set );

	destroy_descriptor_set_impl( device, set );
}

void
update_descriptor_set( const struct Device*          device,
                       struct DescriptorSet*         set,
                       u32                           count,
                       const struct DescriptorWrite* writes )
{
	FT_ASSERT( device );
	FT_ASSERT( set );
	FT_ASSERT( count );
	FT_ASSERT( writes );

	update_descriptor_set_impl( device, set, count, writes );
}
