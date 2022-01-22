#include <iostream>

#include "fluent/fluent.hpp"

using namespace fluent;

struct Vertex
{
    Vector3 position;
    Vector3 normal;
    Vector2 uv;
};

struct CameraUBO
{
    Matrix4 projection;
    Matrix4 view;
    Matrix4 model;
};

struct PushConstantBlock
{
    f32 time;
    f32 mouse_x;
    f32 mouse_y;
};

const InputSystem* input_system = nullptr;

static constexpr u32 FRAME_COUNT = 2;
u32                  frame_index = 0;

Renderer    renderer;
Device      device;
Queue       queue;
CommandPool command_pool;
Semaphore   image_available_semaphores[ FRAME_COUNT ];
Semaphore   rendering_finished_semaphores[ FRAME_COUNT ];
Fence       in_flight_fences[ FRAME_COUNT ];
bool        command_buffers_recorded[ FRAME_COUNT ];

Swapchain     swapchain;
CommandBuffer command_buffers[ FRAME_COUNT ];

DescriptorSetLayout descriptor_set_layout;
Pipeline            pipeline;

Image output_texture;

Buffer uniform_buffer;

DescriptorSet descriptor_set;

void create_output_texture()
{
    ImageDesc image_desc{};
    image_desc.layer_count     = 1;
    image_desc.mip_levels      = 1;
    image_desc.depth           = 1;
    image_desc.format          = Format::eR8G8B8A8Unorm;
    image_desc.width           = 1024;
    image_desc.height          = 1024;
    image_desc.descriptor_type = DescriptorType::eSampledImage | DescriptorType::eStorageImage;

    output_texture = create_image(device, image_desc);

    // TODO: Organize barriers
    ImageBarrier image_barrier{};
    image_barrier.image     = &output_texture;
    image_barrier.src_queue = &queue;
    image_barrier.dst_queue = &queue;
    image_barrier.old_state = ResourceState::eUndefined;
    image_barrier.new_state = ResourceState::eGeneral;

    begin_command_buffer(command_buffers[ 0 ]);
    cmd_barrier(command_buffers[ 0 ], 0, nullptr, 1, &image_barrier);
    end_command_buffer(command_buffers[ 0 ]);
    immediate_submit(queue, command_buffers[ 0 ]);
}

void on_init()
{
    app_set_shaders_directory("../../../examples/shaders/03_compute");
    app_set_textures_directory("../../../examples/textures");

    RendererDesc renderer_desc{};
    renderer_desc.vulkan_allocator = nullptr;
    renderer                       = create_renderer(renderer_desc);

    DeviceDesc device_desc{};
    device_desc.frame_in_use_count = 2;
    device                         = create_device(renderer, device_desc);

    QueueDesc queue_desc{};
    queue_desc.queue_type = QueueType::eGraphics;
    queue                 = get_queue(device, queue_desc);

    CommandPoolDesc command_pool_desc{};
    command_pool_desc.queue = &queue;
    command_pool            = create_command_pool(device, command_pool_desc);

    allocate_command_buffers(device, command_pool, FRAME_COUNT, command_buffers);

    for (u32 i = 0; i < FRAME_COUNT; ++i)
    {
        image_available_semaphores[ i ]    = create_semaphore(device);
        rendering_finished_semaphores[ i ] = create_semaphore(device);
        in_flight_fences[ i ]              = create_fence(device);
        command_buffers_recorded[ i ]      = false;
    }

    SwapchainDesc swapchain_desc{};
    swapchain_desc.width           = window_get_width(get_app_window());
    swapchain_desc.height          = window_get_height(get_app_window());
    swapchain_desc.queue           = &queue;
    swapchain_desc.min_image_count = FRAME_COUNT;

    swapchain = create_swapchain(device, swapchain_desc);

    Shader shader = create_shader(device, "main.comp.glsl.spv", ShaderStage::eCompute);

    descriptor_set_layout = create_descriptor_set_layout(device, 1, &shader);

    PipelineDesc pipeline_desc{};
    pipeline_desc.descriptor_set_layout = &descriptor_set_layout;

    pipeline = create_compute_pipeline(device, pipeline_desc);

    destroy_shader(device, shader);

    BufferDesc buffer_desc{};
    buffer_desc.size            = sizeof(CameraUBO);
    buffer_desc.descriptor_type = DescriptorType::eUniformBuffer;

    uniform_buffer = create_buffer(device, buffer_desc);

    create_output_texture();

    DescriptorSetDesc descriptor_set_desc{};
    descriptor_set_desc.descriptor_set_layout = &descriptor_set_layout;

    descriptor_set = create_descriptor_set(device, descriptor_set_desc);

    DescriptorImageDesc descriptor_image_desc{};
    descriptor_image_desc.sampler        = nullptr;
    descriptor_image_desc.resource_state = ResourceState::eGeneral;
    descriptor_image_desc.image          = &output_texture;

    DescriptorWriteDesc descriptor_write_desc{};
    descriptor_write_desc.descriptor_type        = DescriptorType::eStorageImage;
    descriptor_write_desc.binding                = 0;
    descriptor_write_desc.descriptor_count       = 1;
    descriptor_write_desc.descriptor_image_descs = &descriptor_image_desc;

    update_descriptor_set(device, descriptor_set, 1, &descriptor_write_desc);

    input_system = get_app_input_system();
}

void on_resize(u32 width, u32 height)
{
    queue_wait_idle(queue);
    resize_swapchain(device, swapchain, width, height);
}

void on_update(f32 delta_time)
{
    if (!command_buffers_recorded[ frame_index ])
    {
        wait_for_fences(device, 1, &in_flight_fences[ frame_index ]);
        reset_fences(device, 1, &in_flight_fences[ frame_index ]);
        command_buffers_recorded[ frame_index ] = true;
    }

    u32 image_index = 0;
    acquire_next_image(device, swapchain, image_available_semaphores[ frame_index ], {}, image_index);

    auto& cmd = command_buffers[ frame_index ];

    begin_command_buffer(cmd);

    cmd_set_viewport(cmd, 0, 0, swapchain.width, swapchain.height, 0.0f, 1.0f);
    cmd_set_scissor(cmd, 0, 0, swapchain.width, swapchain.height);
    cmd_bind_pipeline(cmd, pipeline);
    cmd_bind_descriptor_set(cmd, descriptor_set, pipeline);

    PushConstantBlock pcb;
    pcb.time    = get_time() / 400.0f;
    pcb.mouse_x = get_mouse_pos_x(input_system);
    pcb.mouse_y = get_mouse_pos_y(input_system);
    cmd_push_constants(cmd, pipeline, 0, sizeof(PushConstantBlock), &pcb);
    cmd_dispatch(cmd, output_texture.width / 16, output_texture.height / 16, 1);

    cmd_blit_image(
        cmd, output_texture, ResourceState::eGeneral, swapchain.images[ image_index ], ResourceState::eUndefined,
        Filter::eLinear);

    ImageBarrier to_present_barrier{};
    to_present_barrier.src_queue = &queue;
    to_present_barrier.dst_queue = &queue;
    to_present_barrier.image     = &swapchain.images[ image_index ];
    to_present_barrier.old_state = ResourceState::eTransferDst;
    to_present_barrier.new_state = ResourceState::ePresent;

    cmd_barrier(cmd, 0, nullptr, 1, &to_present_barrier);

    ImageBarrier to_storage_barrier{};
    to_storage_barrier.src_queue = &queue;
    to_storage_barrier.dst_queue = &queue;
    to_storage_barrier.image     = &output_texture;
    to_storage_barrier.old_state = ResourceState::eTransferSrc;
    to_storage_barrier.new_state = ResourceState::eGeneral;

    cmd_barrier(cmd, 0, nullptr, 1, &to_storage_barrier);

    end_command_buffer(cmd);

    QueueSubmitDesc queue_submit_desc{};
    queue_submit_desc.wait_semaphore_count   = 1;
    queue_submit_desc.wait_semaphores        = &image_available_semaphores[ frame_index ];
    queue_submit_desc.command_buffer_count   = 1;
    queue_submit_desc.command_buffers        = &cmd;
    queue_submit_desc.signal_semaphore_count = 1;
    queue_submit_desc.signal_semaphores      = &rendering_finished_semaphores[ frame_index ];
    queue_submit_desc.signal_fence           = &in_flight_fences[ frame_index ];

    queue_submit(queue, queue_submit_desc);

    QueuePresentDesc queue_present_desc{};
    queue_present_desc.wait_semaphore_count = 1;
    queue_present_desc.wait_semaphores      = &rendering_finished_semaphores[ frame_index ];
    queue_present_desc.swapchain            = &swapchain;
    queue_present_desc.image_index          = image_index;

    queue_present(queue, queue_present_desc);

    command_buffers_recorded[ frame_index ] = false;
    frame_index                             = (frame_index + 1) % FRAME_COUNT;
}

void on_shutdown()
{
    device_wait_idle(device);
    destroy_image(device, output_texture);
    destroy_buffer(device, uniform_buffer);
    destroy_pipeline(device, pipeline);
    destroy_descriptor_set_layout(device, descriptor_set_layout);
    destroy_swapchain(device, swapchain);
    for (u32 i = 0; i < FRAME_COUNT; ++i)
    {
        destroy_semaphore(device, image_available_semaphores[ i ]);
        destroy_semaphore(device, rendering_finished_semaphores[ i ]);
        destroy_fence(device, in_flight_fences[ i ]);
    }
    free_command_buffers(device, command_pool, FRAME_COUNT, command_buffers);
    destroy_command_pool(device, command_pool);
    destroy_device(device);
    destroy_renderer(renderer);
}

int main(int argc, char** argv)
{
    ApplicationConfig config;
    config.argc        = argc;
    config.argv        = argv;
    config.title       = "TestApp";
    config.x           = 0;
    config.y           = 0;
    config.width       = 800;
    config.height      = 600;
    config.log_level   = LogLevel::eTrace;
    config.on_init     = on_init;
    config.on_update   = on_update;
    config.on_resize   = on_resize;
    config.on_shutdown = on_shutdown;

    app_init(&config);
    app_run();
    app_shutdown();

    return 0;
}
