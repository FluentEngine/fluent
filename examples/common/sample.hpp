#include "fluent/fluent.hpp"

using namespace fluent;

void init_sample();
void update_sample( CommandBuffer* cmd, f32 delta_time );
void resize_sample( u32 width, u32 height );
void shutdown_sample();

static constexpr u32 FRAME_COUNT = 2;
u32                  frame_index = 0;

RendererBackend* backend;
Device*          device;
Queue*           queue;
CommandPool*     command_pool;
Semaphore*       image_available_semaphores[ FRAME_COUNT ];
Semaphore*       rendering_finished_semaphores[ FRAME_COUNT ];
Fence*           in_flight_fences[ FRAME_COUNT ];
bool             command_buffers_recorded[ FRAME_COUNT ];

Swapchain*     swapchain;
CommandBuffer* command_buffers[ FRAME_COUNT ];

Image*       depth_image;
RenderPass** render_passes;

UiContext* ui;

static Shader* load_shader_from_file( const std::string& filename )
{
    std::string ext = filename.substr( filename.find_last_of( "." ) );
    ShaderStage stage;
    if ( ext == ".vert" )
        stage = ShaderStage::eVertex;
    if ( ext == ".frag" )
        stage = ShaderStage::eFragment;
    if ( ext == ".comp" )
        stage = ShaderStage::eCompute;

    auto       code = read_file_binary( FileSystem::get_shaders_directory() +
                                  filename + ".glsl.spv" );
    ShaderDesc desc {};
    desc.bytecode      = code.data();
    desc.bytecode_size = code.size() * sizeof( code[ 0 ] );
    desc.stage         = stage;

    Shader* shader = nullptr;
    create_shader( device, &desc, &shader );
    return shader;
}

static Image* load_image_from_file( const std::string& filename, b32 flip )
{
    u64       size = 0;
    void*     data = nullptr;
    ImageDesc image_desc =
        read_image_data( FileSystem::get_textures_directory() + filename,
                         flip,
                         &size,
                         &data );
    image_desc.format          = Format::eR8G8B8A8Srgb;
    image_desc.descriptor_type = DescriptorType::eSampledImage;
    Image* image;
    create_image( device, &image_desc, &image );
    ResourceLoader::upload_image( image, size, data );

    return image;
}

void create_depth_image( u32 width, u32 height )
{
    ImageDesc depth_image_desc {};
    depth_image_desc.width           = width;
    depth_image_desc.height          = height;
    depth_image_desc.depth           = 1;
    depth_image_desc.format          = Format::eD32Sfloat;
    depth_image_desc.layer_count     = 1;
    depth_image_desc.mip_levels      = 1;
    depth_image_desc.sample_count    = SampleCount::e1;
    depth_image_desc.descriptor_type = DescriptorType::eDepthStencilAttachment;

    create_image( device, &depth_image_desc, &depth_image );

    ImageBarrier barrier {};
    barrier.image     = depth_image;
    barrier.old_state = ResourceState::eUndefined;
    barrier.new_state = ResourceState::eDepthStencilWrite;
    barrier.src_queue = queue;
    barrier.dst_queue = queue;

    begin_command_buffer( command_buffers[ 0 ] );
    cmd_barrier( command_buffers[ 0 ], 0, nullptr, 1, &barrier );
    end_command_buffer( command_buffers[ 0 ] );
    immediate_submit( queue, command_buffers[ 0 ] );
}

void on_init()
{
    FileSystem::set_shaders_directory(
        "../../../examples/shaders/" SAMPLE_NAME );
    FileSystem::set_textures_directory( "../../../examples/textures/" );

    RendererBackendDesc backend_desc {};
    create_renderer_backend( &backend_desc, &backend );

    DeviceDesc device_desc {};
    device_desc.frame_in_use_count = 2;
    create_device( backend, &device_desc, &device );

    QueueDesc queue_desc {};
    queue_desc.queue_type = QueueType::eGraphics;
    create_queue( device, &queue_desc, &queue );

    CommandPoolDesc command_pool_desc {};
    command_pool_desc.queue = queue;
    create_command_pool( device, &command_pool_desc, &command_pool );

    create_command_buffers( device,
                            command_pool,
                            FRAME_COUNT,
                            command_buffers );

    for ( u32 i = 0; i < FRAME_COUNT; ++i )
    {
        create_semaphore( device, &image_available_semaphores[ i ] );
        create_semaphore( device, &rendering_finished_semaphores[ i ] );
        create_fence( device, &in_flight_fences[ i ] );
        command_buffers_recorded[ i ] = false;
    }

    SwapchainDesc swapchain_desc {};
    swapchain_desc.width           = window_get_width( get_app_window() );
    swapchain_desc.height          = window_get_height( get_app_window() );
    swapchain_desc.format          = Format::eB8G8R8A8Srgb;
    swapchain_desc.vsync           = true;
    swapchain_desc.queue           = queue;
    swapchain_desc.min_image_count = FRAME_COUNT;

    create_swapchain( device, &swapchain_desc, &swapchain );

    create_depth_image( swapchain->width, swapchain->height );

    RenderPassDesc render_pass_desc {};
    render_pass_desc.width                          = swapchain->width;
    render_pass_desc.height                         = swapchain->height;
    render_pass_desc.color_attachment_count         = 1;
    render_pass_desc.color_attachment_load_ops[ 0 ] = AttachmentLoadOp::eClear;
    render_pass_desc.color_image_states[ 0 ] = ResourceState::eColorAttachment;
    render_pass_desc.depth_stencil_state   = ResourceState::eDepthStencilWrite;
    render_pass_desc.depth_stencil_load_op = AttachmentLoadOp::eClear;
    render_pass_desc.depth_stencil         = depth_image;

    render_passes = new RenderPass*[ swapchain->image_count ];

    for ( uint32_t i = 0; i < swapchain->image_count; i++ )
    {
        render_pass_desc.color_attachments[ 0 ] = swapchain->images[ i ];
        create_render_pass( device, &render_pass_desc, &render_passes[ i ] );
    }

    ResourceLoader::init( device );

    UiDesc ui_desc {};
    ui_desc.backend            = backend;
    ui_desc.device             = device;
    ui_desc.min_image_count    = swapchain->min_image_count;
    ui_desc.image_count        = swapchain->image_count;
    ui_desc.in_fly_frame_count = FRAME_COUNT;
    ui_desc.queue              = queue;
    ui_desc.render_pass        = render_passes[ 0 ];
    ui_desc.window             = get_app_window();

    create_ui_context( &ui_desc, &ui );

    init_sample();
}

void on_resize( u32 width, u32 height )
{
    queue_wait_idle( queue );
    destroy_image( device, depth_image );

    resize_swapchain( device, swapchain, width, height );

    create_depth_image( width, height );

    RenderPassDesc render_pass_desc {};
    render_pass_desc.width                          = width;
    render_pass_desc.height                         = height;
    render_pass_desc.color_attachment_count         = 1;
    render_pass_desc.color_attachment_load_ops[ 0 ] = AttachmentLoadOp::eClear;
    render_pass_desc.color_image_states[ 0 ] = ResourceState::eColorAttachment;
    render_pass_desc.depth_stencil_state   = ResourceState::eDepthStencilWrite;
    render_pass_desc.depth_stencil_load_op = AttachmentLoadOp::eClear;
    render_pass_desc.depth_stencil         = depth_image;

    for ( uint32_t i = 0; i < swapchain->image_count; i++ )
    {
        render_pass_desc.color_attachments[ 0 ] = swapchain->images[ i ];
        update_render_pass( device, render_passes[ i ], &render_pass_desc );
    }

    resize_sample( width, height );
}

u32 begin_frame()
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
                        nullptr,
                        &image_index );

    auto& cmd = command_buffers[ frame_index ];

    begin_command_buffer( cmd );

    ImageBarrier to_clear_barrier {};
    to_clear_barrier.src_queue = queue;
    to_clear_barrier.dst_queue = queue;
    to_clear_barrier.image     = swapchain->images[ image_index ];
    to_clear_barrier.old_state = ResourceState::eUndefined;
    to_clear_barrier.new_state = ResourceState::eColorAttachment;

    cmd_barrier( cmd, 0, nullptr, 1, &to_clear_barrier );

    RenderPassBeginDesc render_pass_begin_desc {};
    render_pass_begin_desc.render_pass = render_passes[ image_index ];
    render_pass_begin_desc.clear_values[ 0 ].color[ 0 ] = 1.0f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 1 ] = 0.8f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 2 ] = 0.4f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 3 ] = 1.0f;

    cmd_begin_render_pass( cmd, &render_pass_begin_desc );
    cmd_set_viewport( cmd,
                      0,
                      0,
                      swapchain->width,
                      swapchain->height,
                      0.0f,
                      1.0f );
    cmd_set_scissor( cmd, 0, 0, swapchain->width, swapchain->height );

    return image_index;
}

void end_frame( u32 image_index )
{
    auto& cmd = command_buffers[ frame_index ];

    ui_begin_frame();

    ImGuiStyle* style              = &ImGui::GetStyle();
    auto        old_color          = style->Colors[ ImGuiCol_Text ];
    style->Colors[ ImGuiCol_Text ] = ImVec4( 0.0f, 0.0f, 0.0f, 1.00f );

    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoBackground |
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove;

    bool open_ptr = true;

    ImGui::Begin( "Performance", &open_ptr, window_flags );
    ImGui::Text( "FPS: %f", ImGui::GetIO().Framerate );
    ImGui::End();

    style->Colors[ ImGuiCol_Text ] = old_color;

    ui_end_frame( cmd );
    cmd_end_render_pass( cmd );

    ImageBarrier to_present_barrier {};
    to_present_barrier.src_queue = queue;
    to_present_barrier.dst_queue = queue;
    to_present_barrier.image     = swapchain->images[ image_index ];
    to_present_barrier.old_state = ResourceState::eColorAttachment;
    to_present_barrier.new_state = ResourceState::ePresent;

    cmd_barrier( cmd, 0, nullptr, 1, &to_present_barrier );

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

void on_update( f32 delta_time )
{
    auto& cmd = command_buffers[ frame_index ];
    update_sample( cmd, delta_time );
}

void on_shutdown()
{
    device_wait_idle( device );
    shutdown_sample();
    destroy_ui_context( device, ui );
    ResourceLoader::shutdown();
    destroy_image( device, depth_image );
    for ( uint32_t i = 0; i < swapchain->image_count; i++ )
    {
        destroy_render_pass( device, render_passes[ i ] );
    }
    delete[] render_passes;
    destroy_swapchain( device, swapchain );
    for ( u32 i = 0; i < FRAME_COUNT; ++i )
    {
        destroy_semaphore( device, image_available_semaphores[ i ] );
        destroy_semaphore( device, rendering_finished_semaphores[ i ] );
        destroy_fence( device, in_flight_fences[ i ] );
    }
    destroy_command_buffers( device,
                             command_pool,
                             FRAME_COUNT,
                             command_buffers );
    destroy_command_pool( device, command_pool );
    destroy_queue( queue );
    destroy_device( device );
    destroy_renderer_backend( backend );
}

int main( int argc, char** argv )
{
    ApplicationConfig config;
    config.argc        = argc;
    config.argv        = argv;
    config.window_desc = { SAMPLE_NAME, 100, 100, 1400, 900, false };
    config.log_level   = LogLevel::eTrace;
    config.on_init     = on_init;
    config.on_update   = on_update;
    config.on_resize   = on_resize;
    config.on_shutdown = on_shutdown;

    app_init( &config );
    app_run();
    app_shutdown();

    return 0;
}
