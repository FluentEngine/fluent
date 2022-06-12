#pragma once

#include "renderer_backend.h"

#undef MemoryBarrier

#define FT_INIT_INTERNAL( name, ptr, type )                                    \
	struct type* name      = calloc( 1, sizeof( struct type ) );               \
	name->interface.handle = name;                                             \
	ptr                    = &name->interface

#define FT_FROM_HANDLE( name, interface, impl )                                \
	struct impl* name = interface->handle

#ifdef __cplusplus
extern "C"
{
#endif

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_renderer_backend,
	                          struct RendererBackend* backend );

	DECLARE_FUNCTION_POINTER( void,
	                          create_device,
	                          const struct RendererBackend* backend,
	                          const struct DeviceInfo*      info,
	                          struct Device**               device );

	DECLARE_FUNCTION_POINTER( void, destroy_device, struct Device* device );

	DECLARE_FUNCTION_POINTER( void,
	                          create_queue,
	                          const struct Device*    device,
	                          const struct QueueInfo* info,
	                          struct Queue**          queue );

	DECLARE_FUNCTION_POINTER( void, destroy_queue, struct Queue* queue );

	DECLARE_FUNCTION_POINTER( void,
	                          queue_wait_idle,
	                          const struct Queue* queue );

	DECLARE_FUNCTION_POINTER( void,
	                          queue_submit,
	                          const struct Queue*           queue,
	                          const struct QueueSubmitInfo* info );

	DECLARE_FUNCTION_POINTER( void,
	                          immediate_submit,
	                          const struct Queue*   queue,
	                          struct CommandBuffer* cmd );

	DECLARE_FUNCTION_POINTER( void,
	                          queue_present,
	                          const struct Queue*            queue,
	                          const struct QueuePresentInfo* info );

	DECLARE_FUNCTION_POINTER( void,
	                          create_semaphore,
	                          const struct Device* device,
	                          struct Semaphore**   semaphore );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_semaphore,
	                          const struct Device* device,
	                          struct Semaphore*    semaphore );

	DECLARE_FUNCTION_POINTER( void,
	                          create_fence,
	                          const struct Device* device,
	                          struct Fence**       fence );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_fence,
	                          const struct Device* device,
	                          struct Fence*        fence );

	DECLARE_FUNCTION_POINTER( void,
	                          wait_for_fences,
	                          const struct Device* device,
	                          u32                  count,
	                          struct Fence**       fences );

	DECLARE_FUNCTION_POINTER( void,
	                          reset_fences,
	                          const struct Device* device,
	                          u32                  count,
	                          struct Fence**       fences );

	DECLARE_FUNCTION_POINTER( void,
	                          create_swapchain,
	                          const struct Device*        device,
	                          const struct SwapchainInfo* info,
	                          struct Swapchain**          swapchain );

	DECLARE_FUNCTION_POINTER( void,
	                          resize_swapchain,
	                          const struct Device* device,
	                          struct Swapchain*    swapchain,
	                          u32                  width,
	                          u32                  height );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_swapchain,
	                          const struct Device* device,
	                          struct Swapchain*    swapchain );

	DECLARE_FUNCTION_POINTER( void,
	                          create_command_pool,
	                          const struct Device*          device,
	                          const struct CommandPoolInfo* info,
	                          struct CommandPool**          command_pool );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_command_pool,
	                          const struct Device* device,
	                          struct CommandPool*  command_pool );

	DECLARE_FUNCTION_POINTER( void,
	                          create_command_buffers,
	                          const struct Device*      device,
	                          const struct CommandPool* command_pool,
	                          u32                       count,
	                          struct CommandBuffer**    command_buffers );

	DECLARE_FUNCTION_POINTER( void,
	                          free_command_buffers,
	                          const struct Device*      device,
	                          const struct CommandPool* command_pool,
	                          u32                       count,
	                          struct CommandBuffer**    command_buffers );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_command_buffers,
	                          const struct Device*      device,
	                          const struct CommandPool* command_pool,
	                          u32                       count,
	                          struct CommandBuffer**    command_buffers );

	DECLARE_FUNCTION_POINTER( void,
	                          begin_command_buffer,
	                          const struct CommandBuffer* cmd );

	DECLARE_FUNCTION_POINTER( void,
	                          end_command_buffer,
	                          const struct CommandBuffer* cmd );

	DECLARE_FUNCTION_POINTER( void,
	                          acquire_next_image,
	                          const struct Device*    device,
	                          const struct Swapchain* swapchain,
	                          const struct Semaphore* semaphore,
	                          const struct Fence*     fence,
	                          u32*                    image_index );

	DECLARE_FUNCTION_POINTER( void,
	                          create_shader,
	                          const struct Device* device,
	                          struct ShaderInfo*   info,
	                          struct Shader**      shader );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_shader,
	                          const struct Device* device,
	                          struct Shader*       shader );

	DECLARE_FUNCTION_POINTER(
	    void,
	    create_descriptor_set_layout,
	    const struct Device*         device,
	    struct Shader*               shader,
	    struct DescriptorSetLayout** descriptor_set_layout );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_descriptor_set_layout,
	                          const struct Device*        device,
	                          struct DescriptorSetLayout* layout );

	DECLARE_FUNCTION_POINTER( void,
	                          create_compute_pipeline,
	                          const struct Device*       device,
	                          const struct PipelineInfo* info,
	                          struct Pipeline**          pipeline );

	DECLARE_FUNCTION_POINTER( void,
	                          create_graphics_pipeline,
	                          const struct Device*       device,
	                          const struct PipelineInfo* info,
	                          struct Pipeline**          pipeline );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_pipeline,
	                          const struct Device* device,
	                          struct Pipeline*     pipeline );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_begin_render_pass,
	                          const struct CommandBuffer*       cmd,
	                          const struct RenderPassBeginInfo* info );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_end_render_pass,
	                          const struct CommandBuffer* cmd );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_barrier,
	                          const struct CommandBuffer* cmd,
	                          u32                         memory_barriers_count,
	                          const struct MemoryBarrier* memory_barrier,
	                          u32                         buffer_barriers_count,
	                          const struct BufferBarrier* buffer_barriers,
	                          u32                         image_barriers_count,
	                          const struct ImageBarrier*  image_barriers );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_set_scissor,
	                          const struct CommandBuffer* cmd,
	                          i32                         x,
	                          i32                         y,
	                          u32                         width,
	                          u32                         height );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_set_viewport,
	                          const struct CommandBuffer* cmd,
	                          f32                         x,
	                          f32                         y,
	                          f32                         width,
	                          f32                         height,
	                          f32                         min_depth,
	                          f32                         max_depth );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_bind_pipeline,
	                          const struct CommandBuffer* cmd,
	                          const struct Pipeline*      pipeline );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_draw,
	                          const struct CommandBuffer* cmd,
	                          u32                         vertex_count,
	                          u32                         instance_count,
	                          u32                         first_vertex,
	                          u32                         first_instance );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_draw_indexed,
	                          const struct CommandBuffer* cmd,
	                          u32                         index_count,
	                          u32                         instance_count,
	                          u32                         first_index,
	                          i32                         vertex_offset,
	                          u32                         first_instance );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_bind_vertex_buffer,
	                          const struct CommandBuffer* cmd,
	                          const struct Buffer*        buffer,
	                          const u64                   offset );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_bind_index_buffer_u16,
	                          const struct CommandBuffer* cmd,
	                          const struct Buffer*        buffer,
	                          const u64                   offset );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_bind_index_buffer_u32,
	                          const struct CommandBuffer* cmd,
	                          const struct Buffer*        buffer,
	                          const u64                   offset );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_copy_buffer,
	                          const struct CommandBuffer* cmd,
	                          const struct Buffer*        src,
	                          u64                         src_offset,
	                          struct Buffer*              dst,
	                          u64                         dst_offset,
	                          u64                         size );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_copy_buffer_to_image,
	                          const struct CommandBuffer* cmd,
	                          const struct Buffer*        src,
	                          u64                         src_offset,
	                          struct Image*               dst );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_bind_descriptor_set,
	                          const struct CommandBuffer* cmd,
	                          u32                         first_set,
	                          const struct DescriptorSet* set,
	                          const struct Pipeline*      pipeline );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_dispatch,
	                          const struct CommandBuffer* cmd,
	                          u32                         group_count_x,
	                          u32                         group_count_y,
	                          u32                         group_count_z );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_push_constants,
	                          const struct CommandBuffer* cmd,
	                          const struct Pipeline*      pipeline,
	                          u32                         offset,
	                          u32                         size,
	                          const void*                 data );

	DECLARE_FUNCTION_POINTER( void,
	                          create_buffer,
	                          const struct Device*     device,
	                          const struct BufferInfo* info,
	                          struct Buffer**          buffer );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_buffer,
	                          const struct Device* device,
	                          struct Buffer*       buffer );

	DECLARE_FUNCTION_POINTER( void*,
	                          map_memory,
	                          const struct Device* device,
	                          struct Buffer*       buffer );

	DECLARE_FUNCTION_POINTER( void,
	                          unmap_memory,
	                          const struct Device* device,
	                          struct Buffer*       buffer );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_draw_indexed_indirect,
	                          const struct CommandBuffer* cmd,
	                          const struct Buffer*        buffer,
	                          u64                         offset,
	                          u32                         draw_count,
	                          u32                         stride );

	DECLARE_FUNCTION_POINTER( void,
	                          create_sampler,
	                          const struct Device*      device,
	                          const struct SamplerInfo* info,
	                          struct Sampler**          sampler );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_sampler,
	                          const struct Device* device,
	                          struct Sampler*      sampler );

	DECLARE_FUNCTION_POINTER( void,
	                          create_image,
	                          const struct Device*    device,
	                          const struct ImageInfo* info,
	                          struct Image**          image );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_image,
	                          const struct Device* device,
	                          struct Image*        image );

	DECLARE_FUNCTION_POINTER( void,
	                          create_descriptor_set,
	                          const struct Device*            device,
	                          const struct DescriptorSetInfo* info,
	                          struct DescriptorSet**          descriptor_set );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_descriptor_set,
	                          const struct Device*  device,
	                          struct DescriptorSet* set );

	DECLARE_FUNCTION_POINTER( void,
	                          update_descriptor_set,
	                          const struct Device*          device,
	                          struct DescriptorSet*         set,
	                          u32                           count,
	                          const struct DescriptorWrite* writes );

#ifdef __cplusplus
}
#endif
