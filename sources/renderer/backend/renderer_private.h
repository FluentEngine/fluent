#pragma once

#include "renderer_backend.h"

#define FT_INIT_INTERNAL( name, ptr, type )                                    \
	struct type* name      = calloc( 1, sizeof( struct type ) );               \
	name->interface.handle = name;                                             \
	ptr                    = &name->interface

#define FT_FROM_HANDLE( name, interface, impl )                                \
	struct impl* name = interface->handle

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_destroy_renderer_backend,
                             struct ft_renderer_backend* backend );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_create_device,
                             const struct ft_renderer_backend* backend,
                             const struct ft_device_info*      info,
                             struct ft_device**                device );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_destroy_device,
                             struct ft_device* device );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_create_queue,
                             const struct ft_device*     device,
                             const struct ft_queue_info* info,
                             struct ft_queue**           queue );

FT_DECLARE_FUNCTION_POINTER( void, ft_destroy_queue, struct ft_queue* queue );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_queue_wait_idle,
                             const struct ft_queue* queue );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_queue_submit,
                             const struct ft_queue*             queue,
                             const struct ft_queue_submit_info* info );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_immediate_submit,
                             const struct ft_queue*    queue,
                             struct ft_command_buffer* cmd );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_queue_present,
                             const struct ft_queue*              queue,
                             const struct ft_queue_present_info* info );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_create_semaphore,
                             const struct ft_device* device,
                             struct ft_semaphore**   semaphore );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_destroy_semaphore,
                             const struct ft_device* device,
                             struct ft_semaphore*    semaphore );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_create_fence,
                             const struct ft_device* device,
                             struct ft_fence**       fence );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_destroy_fence,
                             const struct ft_device* device,
                             struct ft_fence*        fence );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_wait_for_fences,
                             const struct ft_device* device,
                             uint32_t                count,
                             struct ft_fence**       fences );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_reset_fences,
                             const struct ft_device* device,
                             uint32_t                count,
                             struct ft_fence**       fences );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_create_swapchain,
                             const struct ft_device*         device,
                             const struct ft_swapchain_info* info,
                             struct ft_swapchain**           swapchain );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_resize_swapchain,
                             const struct ft_device* device,
                             struct ft_swapchain*    swapchain,
                             uint32_t                width,
                             uint32_t                height );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_destroy_swapchain,
                             const struct ft_device* device,
                             struct ft_swapchain*    swapchain );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_create_command_pool,
                             const struct ft_device*            device,
                             const struct ft_command_pool_info* info,
                             struct ft_command_pool**           command_pool );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_destroy_command_pool,
                             const struct ft_device* device,
                             struct ft_command_pool* command_pool );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_create_command_buffers,
                             const struct ft_device*       device,
                             const struct ft_command_pool* command_pool,
                             uint32_t                      count,
                             struct ft_command_buffer**    command_buffers );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_free_command_buffers,
                             const struct ft_device*       device,
                             const struct ft_command_pool* command_pool,
                             uint32_t                      count,
                             struct ft_command_buffer**    command_buffers );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_destroy_command_buffers,
                             const struct ft_device*       device,
                             const struct ft_command_pool* command_pool,
                             uint32_t                      count,
                             struct ft_command_buffer**    command_buffers );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_begin_command_buffer,
                             const struct ft_command_buffer* cmd );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_end_command_buffer,
                             const struct ft_command_buffer* cmd );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_acquire_next_image,
                             const struct ft_device*    device,
                             const struct ft_swapchain* swapchain,
                             const struct ft_semaphore* semaphore,
                             const struct ft_fence*     fence,
                             uint32_t*                  image_index );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_create_shader,
                             const struct ft_device* device,
                             struct ft_shader_info*  info,
                             struct ft_shader**      shader );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_destroy_shader,
                             const struct ft_device* device,
                             struct ft_shader*       shader );

FT_DECLARE_FUNCTION_POINTER(
    void,
    ft_create_descriptor_set_layout,
    const struct ft_device*           device,
    struct ft_shader*                 shader,
    struct ft_descriptor_set_layout** descriptor_set_layout );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_destroy_descriptor_set_layout,
                             const struct ft_device*          device,
                             struct ft_descriptor_set_layout* layout );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_create_pipeline,
                             const struct ft_device*        device,
                             const struct ft_pipeline_info* info,
                             struct ft_pipeline**           pipeline );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_destroy_pipeline,
                             const struct ft_device* device,
                             struct ft_pipeline*     pipeline );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_cmd_begin_render_pass,
                             const struct ft_command_buffer*         cmd,
                             const struct ft_render_pass_begin_info* info );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_cmd_end_render_pass,
                             const struct ft_command_buffer* cmd );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_cmd_barrier,
                             const struct ft_command_buffer* cmd,
                             uint32_t memory_barriers_count,
                             const struct ft_memory_barrier* memory_barrier,
                             uint32_t buffer_barriers_count,
                             const struct ft_buffer_barrier* buffer_barriers,
                             uint32_t image_barriers_count,
                             const struct ft_image_barrier* image_barriers );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_cmd_set_scissor,
                             const struct ft_command_buffer* cmd,
                             int32_t                         x,
                             int32_t                         y,
                             uint32_t                        width,
                             uint32_t                        height );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_cmd_set_viewport,
                             const struct ft_command_buffer* cmd,
                             float                           x,
                             float                           y,
                             float                           width,
                             float                           height,
                             float                           min_depth,
                             float                           max_depth );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_cmd_bind_pipeline,
                             const struct ft_command_buffer* cmd,
                             const struct ft_pipeline*       pipeline );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_cmd_draw,
                             const struct ft_command_buffer* cmd,
                             uint32_t                        vertex_count,
                             uint32_t                        instance_count,
                             uint32_t                        first_vertex,
                             uint32_t                        first_instance );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_cmd_draw_indexed,
                             const struct ft_command_buffer* cmd,
                             uint32_t                        index_count,
                             uint32_t                        instance_count,
                             uint32_t                        first_index,
                             int32_t                         vertex_offset,
                             uint32_t                        first_instance );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_cmd_bind_vertex_buffer,
                             const struct ft_command_buffer* cmd,
                             const struct ft_buffer*         buffer,
                             const uint64_t                  offset );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_cmd_bind_index_buffer,
                             const struct ft_command_buffer* cmd,
                             const struct ft_buffer*         buffer,
                             const uint64_t                  offset,
                             enum IndexType                  index_type );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_cmd_copy_buffer,
                             const struct ft_command_buffer* cmd,
                             const struct ft_buffer*         src,
                             uint64_t                        src_offset,
                             struct ft_buffer*               dst,
                             uint64_t                        dst_offset,
                             uint64_t                        size );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_cmd_copy_buffer_to_image,
                             const struct ft_command_buffer* cmd,
                             const struct ft_buffer*         src,
                             uint64_t                        src_offset,
                             struct ft_image*                dst );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_cmd_bind_descriptor_set,
                             const struct ft_command_buffer* cmd,
                             uint32_t                        first_set,
                             const struct ft_descriptor_set* set,
                             const struct ft_pipeline*       pipeline );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_cmd_dispatch,
                             const struct ft_command_buffer* cmd,
                             uint32_t                        group_count_x,
                             uint32_t                        group_count_y,
                             uint32_t                        group_count_z );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_cmd_push_constants,
                             const struct ft_command_buffer* cmd,
                             const struct ft_pipeline*       pipeline,
                             uint32_t                        offset,
                             uint32_t                        size,
                             const void*                     data );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_create_buffer,
                             const struct ft_device*      device,
                             const struct ft_buffer_info* info,
                             struct ft_buffer**           buffer );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_destroy_buffer,
                             const struct ft_device* device,
                             struct ft_buffer*       buffer );

FT_DECLARE_FUNCTION_POINTER( void*,
                             ft_map_memory,
                             const struct ft_device* device,
                             struct ft_buffer*       buffer );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_unmap_memory,
                             const struct ft_device* device,
                             struct ft_buffer*       buffer );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_cmd_draw_indexed_indirect,
                             const struct ft_command_buffer* cmd,
                             const struct ft_buffer*         buffer,
                             uint64_t                        offset,
                             uint32_t                        draw_count,
                             uint32_t                        stride );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_create_sampler,
                             const struct ft_device*       device,
                             const struct ft_sampler_info* info,
                             struct ft_sampler**           sampler );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_destroy_sampler,
                             const struct ft_device* device,
                             struct ft_sampler*      sampler );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_create_image,
                             const struct ft_device*     device,
                             const struct ft_image_info* info,
                             struct ft_image**           image );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_destroy_image,
                             const struct ft_device* device,
                             struct ft_image*        image );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_create_descriptor_set,
                             const struct ft_device*              device,
                             const struct ft_descriptor_set_info* info,
                             struct ft_descriptor_set** descriptor_set );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_destroy_descriptor_set,
                             const struct ft_device*   device,
                             struct ft_descriptor_set* set );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_update_descriptor_set,
                             const struct ft_device*           device,
                             struct ft_descriptor_set*         set,
                             uint32_t                          count,
                             const struct ft_descriptor_write* writes );
