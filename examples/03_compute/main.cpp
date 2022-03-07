#define SAMPLE_NAME "03_compute"
#include "../common/sample.hpp"

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

DescriptorSetLayout* descriptor_set_layout;
Pipeline*            pipeline;

Image* output_texture;

Buffer* uniform_buffer;

DescriptorSet* descriptor_set;

void create_output_texture()
{
    ImageDesc image_desc {};
    image_desc.layer_count = 1;
    image_desc.mip_levels  = 1;
    image_desc.depth       = 1;
    image_desc.format      = Format::eR8G8B8A8Unorm;
    image_desc.width       = 1024;
    image_desc.height      = 1024;
    image_desc.descriptor_type =
        DescriptorType::eSampledImage | DescriptorType::eStorageImage;

    create_image( device, &image_desc, &output_texture );

    ImageBarrier image_barrier {};
    image_barrier.image     = output_texture;
    image_barrier.src_queue = queue;
    image_barrier.dst_queue = queue;
    image_barrier.old_state = ResourceState::eUndefined;
    image_barrier.new_state = ResourceState::eGeneral;

    CommandBuffer* cmd = command_buffers[ 0 ];

    begin_command_buffer( cmd );
    cmd_barrier( cmd, 0, nullptr, 1, &image_barrier );
    end_command_buffer( cmd );
    immediate_submit( queue, cmd );
}

void init_sample()
{
    Shader* shader = load_shader_from_file( "main.comp" );

    create_descriptor_set_layout( device, 1, &shader, &descriptor_set_layout );

    PipelineDesc pipeline_desc {};
    pipeline_desc.shader_count          = 1;
    pipeline_desc.shaders[ 0 ]          = shader;
    pipeline_desc.descriptor_set_layout = descriptor_set_layout;

    create_compute_pipeline( device, &pipeline_desc, &pipeline );

    destroy_shader( device, shader );

    BufferDesc buffer_desc {};
    buffer_desc.size            = sizeof( CameraUBO );
    buffer_desc.descriptor_type = DescriptorType::eUniformBuffer;

    create_buffer( device, &buffer_desc, &uniform_buffer );

    create_output_texture();

    DescriptorSetDesc descriptor_set_desc {};
    descriptor_set_desc.descriptor_set_layout = descriptor_set_layout;
    descriptor_set_desc.set                   = 0;

    create_descriptor_set( device, &descriptor_set_desc, &descriptor_set );

    ImageDescriptor image_descriptor {};
    image_descriptor.sampler        = nullptr;
    image_descriptor.resource_state = ResourceState::eGeneral;
    image_descriptor.image          = output_texture;

    DescriptorWrite descriptor_write {};
    descriptor_write.descriptor_type   = DescriptorType::eStorageImage;
    descriptor_write.binding           = 0;
    descriptor_write.descriptor_count  = 1;
    descriptor_write.image_descriptors = &image_descriptor;

    update_descriptor_set( device, descriptor_set, 1, &descriptor_write );

    input_system = get_app_input_system();
}

void resize_sample( u32 width, u32 height ) {}

void update_sample( CommandBuffer* cmd, f32 delta_time )
{
    if ( !command_buffers_recorded[ frame_index ] )
    {
        wait_for_fences( device, 1, &in_flight_fences[ frame_index ] );
        reset_fences( device, 1, &in_flight_fences[ frame_index ] );
        command_buffers_recorded[ frame_index ] = true;
    }

    u32 image_index = 0;
    acquire_next_image( device,
                        swapchain,
                        image_available_semaphores[ frame_index ],
                        {},
                        &image_index );

    begin_command_buffer( cmd );

    cmd_set_viewport( cmd,
                      0,
                      0,
                      swapchain->width,
                      swapchain->height,
                      0.0f,
                      1.0f );

    cmd_set_scissor( cmd, 0, 0, swapchain->width, swapchain->height );
    cmd_bind_pipeline( cmd, pipeline );
    cmd_bind_descriptor_set( cmd, 0, descriptor_set, pipeline );

    PushConstantBlock pcb;
    pcb.time    = get_time() / 400.0f;
    pcb.mouse_x = get_mouse_pos_x( input_system );
    pcb.mouse_y = get_mouse_pos_y( input_system );
    cmd_push_constants( cmd, pipeline, 0, sizeof( PushConstantBlock ), &pcb );
    cmd_dispatch( cmd,
                  output_texture->width / 16,
                  output_texture->height / 16,
                  1 );

    cmd_blit_image( cmd,
                    output_texture,
                    ResourceState::eGeneral,
                    swapchain->images[ image_index ],
                    ResourceState::eUndefined,
                    Filter::eLinear );

    ImageBarrier to_present_barrier {};
    to_present_barrier.src_queue = queue;
    to_present_barrier.dst_queue = queue;
    to_present_barrier.image     = swapchain->images[ image_index ];
    to_present_barrier.old_state = ResourceState::eTransferDst;
    to_present_barrier.new_state = ResourceState::ePresent;

    cmd_barrier( cmd, 0, nullptr, 1, &to_present_barrier );

    ImageBarrier to_storage_barrier {};
    to_storage_barrier.src_queue = queue;
    to_storage_barrier.dst_queue = queue;
    to_storage_barrier.image     = output_texture;
    to_storage_barrier.old_state = ResourceState::eTransferSrc;
    to_storage_barrier.new_state = ResourceState::eGeneral;

    cmd_barrier( cmd, 0, nullptr, 1, &to_storage_barrier );

    end_command_buffer( cmd );

    QueueSubmitDesc queue_submit_desc {};
    queue_submit_desc.wait_semaphore_count = 1;
    queue_submit_desc.wait_semaphores =
        image_available_semaphores[ frame_index ];
    queue_submit_desc.command_buffer_count   = 1;
    queue_submit_desc.command_buffers        = cmd;
    queue_submit_desc.signal_semaphore_count = 1;
    queue_submit_desc.signal_semaphores =
        rendering_finished_semaphores[ frame_index ];
    queue_submit_desc.signal_fence = in_flight_fences[ frame_index ];

    queue_submit( queue, &queue_submit_desc );

    QueuePresentDesc queue_present_desc {};
    queue_present_desc.wait_semaphore_count = 1;
    queue_present_desc.wait_semaphores =
        rendering_finished_semaphores[ frame_index ];
    queue_present_desc.swapchain   = swapchain;
    queue_present_desc.image_index = image_index;

    queue_present( queue, &queue_present_desc );

    command_buffers_recorded[ frame_index ] = false;
    frame_index                             = ( frame_index + 1 ) % FRAME_COUNT;
}

void shutdown_sample()
{
    destroy_image( device, output_texture );
    destroy_buffer( device, uniform_buffer );
    destroy_pipeline( device, pipeline );
    destroy_descriptor_set_layout( device, descriptor_set_layout );
}
