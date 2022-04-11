#include "renderer/renderer_backend.hpp"
#include "renderer/vulkan/vulkan_backend.hpp"
#include "renderer/d3d12/d3d12_backend.hpp"

namespace fluent
{
destroy_renderer_backend_fun      destroy_renderer_backend;
create_device_fun                 create_device;
destroy_device_fun                destroy_device;
create_queue_fun                  create_queue;
destroy_queue_fun                 destroy_queue;
queue_wait_idle_fun               queue_wait_idle;
queue_submit_fun                  queue_submit;
immediate_submit_fun              immediate_submit;
queue_present_fun                 queue_present;
create_semaphore_fun              create_semaphore;
destroy_semaphore_fun             destroy_semaphore;
create_fence_fun                  create_fence;
destroy_fence_fun                 destroy_fence;
wait_for_fences_fun               wait_for_fences;
reset_fences_fun                  reset_fences;
create_swapchain_fun              create_swapchain;
resize_swapchain_fun              resize_swapchain;
destroy_swapchain_fun             destroy_swapchain;
create_command_pool_fun           create_command_pool;
destroy_command_pool_fun          destroy_command_pool;
create_command_buffers_fun        create_command_buffers;
free_command_buffers_fun          free_command_buffers;
destroy_command_buffers_fun       destroy_command_buffers;
begin_command_buffer_fun          begin_command_buffer;
end_command_buffer_fun            end_command_buffer;
acquire_next_image_fun            acquire_next_image;
create_render_pass_fun            create_render_pass;
update_render_pass_fun            update_render_pass;
destroy_render_pass_fun           destroy_render_pass;
create_shader_fun                 create_shader;
destroy_shader_fun                destroy_shader;
create_descriptor_set_layout_fun  create_descriptor_set_layout;
destroy_descriptor_set_layout_fun destroy_descriptor_set_layout;
create_compute_pipeline_fun       create_compute_pipeline;
create_graphics_pipeline_fun      create_graphics_pipeline;
destroy_pipeline_fun              destroy_pipeline;
create_buffer_fun                 create_buffer;
destroy_buffer_fun                destroy_buffer;
map_memory_fun                    map_memory;
unmap_memory_fun                  unmap_memory;
create_sampler_fun                create_sampler;
destroy_sampler_fun               destroy_sampler;
create_image_fun                  create_image;
destroy_image_fun                 destroy_image;
create_descriptor_set_fun         create_descriptor_set;
destroy_descriptor_set_fun        destroy_descriptor_set;
update_descriptor_set_fun         update_descriptor_set;
create_ui_context_fun             create_ui_context;
destroy_ui_context_fun            destroy_ui_context;
ui_begin_frame_fun                ui_begin_frame;
ui_end_frame_fun                  ui_end_frame;
cmd_begin_render_pass_fun         cmd_begin_render_pass;
cmd_end_render_pass_fun           cmd_end_render_pass;
cmd_barrier_fun                   cmd_barrier;
cmd_set_scissor_fun               cmd_set_scissor;
cmd_set_viewport_fun              cmd_set_viewport;
cmd_bind_pipeline_fun             cmd_bind_pipeline;
cmd_draw_fun                      cmd_draw;
cmd_draw_indexed_fun              cmd_draw_indexed;
cmd_bind_vertex_buffer_fun        cmd_bind_vertex_buffer;
cmd_bind_index_buffer_u16_fun     cmd_bind_index_buffer_u16;
cmd_bind_index_buffer_u32_fun     cmd_bind_index_buffer_u32;
cmd_copy_buffer_fun               cmd_copy_buffer;
cmd_copy_buffer_to_image_fun      cmd_copy_buffer_to_image;
cmd_bind_descriptor_set_fun       cmd_bind_descriptor_set;
cmd_dispatch_fun                  cmd_dispatch;
cmd_push_constants_fun            cmd_push_constants;
cmd_blit_image_fun                cmd_blit_image;
cmd_clear_color_image_fun         cmd_clear_color_image;
cmd_draw_indexed_indirect_fun     cmd_draw_indexed_indirect;

read_shader_fun read_shader;

void create_renderer_backend( const RendererBackendDesc* desc,
                              RendererBackend**          backend )
{
    switch ( desc->api )
    {
    case RendererAPI::eVulkan:
    {
#ifdef VULKAN_BACKEND
        vk_create_renderer_backend( desc, backend );
        break;
#endif
    }
    case RendererAPI::eD3D12:
    {
#ifdef D3D12_BACKEND
        d3d12_create_renderer_backend( desc, backend );
        break;
#endif
    }
    default: FT_ASSERT( false && "At least one api should be enabled" );
    }
}
} // namespace fluent
