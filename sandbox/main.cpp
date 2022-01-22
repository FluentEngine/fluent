#include <functional>
#include <chrono>
#include <thread>
#include "fluent/fluent.hpp"
#include "map_generator.hpp"
#include "mesh_generator.hpp"
#include "mesh.hpp"
#include "camera.hpp"
#include "endless_terrain.hpp"

using namespace fluent;

// basic stuff needed for run render loop
static constexpr u32 FRAME_COUNT = 2;
Renderer             renderer;
Device               device;
Queue                queue;
CommandPool          command_pool;
Semaphore            image_available_semaphores[ FRAME_COUNT ];
Semaphore            rendering_finished_semaphores[ FRAME_COUNT ];
Fence                in_flight_fences[ FRAME_COUNT ];
bool                 command_buffers_recorded[ FRAME_COUNT ];
Swapchain            swapchain;
CommandBuffer        command_buffers[ FRAME_COUNT ];
u32                  frame_index = 0;

// debug gui
UiContext ui_context;
// editor
RenderPass editor_render_pass;
Image      editor_image;
Image      editor_depth_image;

// scene
Camera           camera;
CameraController camera_controller;

struct
{
    Matrix4 model;
} pc;

Buffer camera_buffer;

Sampler sampler;

f32                 height_multiplier = 40.0f;
DescriptorSetLayout mesh_dsl;
Pipeline            mesh_pipeline;
DescriptorSet       scene_set;

EndlessTerrain endless_terrain;

// helpers
ImageBarrier create_image_barrier(
    Image& image, ResourceState old_state, ResourceState new_state, Queue& src_queue, Queue& dst_queue)
{
    ImageBarrier barrier{};
    barrier.image     = &image;
    barrier.src_queue = &src_queue;
    barrier.dst_queue = &dst_queue;
    barrier.old_state = old_state;
    barrier.new_state = new_state;

    return barrier;
}

// scene functions
void create_scene();
void destroy_scene();
void update_camera(f32 delta_time);

void create_mesh_pipeline()
{
    Shader shaders[ 2 ] = {};
    shaders[ 0 ]        = create_shader(device, "main.vert.glsl.spv", ShaderStage::eVertex);
    shaders[ 1 ]        = create_shader(device, "main.frag.glsl.spv", ShaderStage::eFragment);

    mesh_dsl = create_descriptor_set_layout(device, 2, shaders);

    PipelineDesc pipeline_desc{};
    pipeline_desc.binding_desc_count            = 1;
    pipeline_desc.binding_descs[ 0 ].binding    = 0;
    pipeline_desc.binding_descs[ 0 ].input_rate = VertexInputRate::eVertex;
    pipeline_desc.binding_descs[ 0 ].stride     = 5 * sizeof(float);

    pipeline_desc.attribute_desc_count          = 2;
    pipeline_desc.attribute_descs[ 0 ].binding  = 0;
    pipeline_desc.attribute_descs[ 0 ].format   = Format::eR32G32B32Sfloat;
    pipeline_desc.attribute_descs[ 0 ].location = 0;
    pipeline_desc.attribute_descs[ 0 ].offset   = 0;
    pipeline_desc.attribute_descs[ 1 ].binding  = 0;
    pipeline_desc.attribute_descs[ 1 ].format   = Format::eR32G32Sfloat;
    pipeline_desc.attribute_descs[ 1 ].location = 1;
    pipeline_desc.attribute_descs[ 1 ].offset   = 3 * sizeof(float);

    pipeline_desc.rasterizer_desc.cull_mode    = CullMode::eBack;
    pipeline_desc.rasterizer_desc.front_face   = FrontFace::eCounterClockwise;
    pipeline_desc.rasterizer_desc.polygon_mode = PolygonMode::eLine;
    pipeline_desc.depth_state_desc.depth_test  = true;
    pipeline_desc.depth_state_desc.depth_write = true;
    pipeline_desc.depth_state_desc.compare_op  = CompareOp::eLess;
    pipeline_desc.descriptor_set_layout        = &mesh_dsl;
    pipeline_desc.render_pass                  = &editor_render_pass;

    mesh_pipeline = create_graphics_pipeline(device, pipeline_desc);

    for (u32 i = 0; i < 2; ++i)
    {
        destroy_shader(device, shaders[ i ]);
    }
}

void destroy_mesh_pipeline()
{
    destroy_pipeline(device, mesh_pipeline);
    destroy_descriptor_set_layout(device, mesh_dsl);
}

void create_camera()
{
    camera.init_camera(Vector3(0.0f, 1.0, 100.0f), Vector3(0.0, 0.0, -1.0), Vector3(0.0, 1.0, 0.0));
    camera_controller.init(get_app_input_system(), camera);

    BufferDesc buffer_desc{};
    buffer_desc.descriptor_type = DescriptorType::eUniformBuffer;
    buffer_desc.data            = &camera.get_data();
    buffer_desc.size            = sizeof(CameraData);

    camera_buffer = create_buffer(device, buffer_desc);
}

void destroy_camera()
{
    destroy_buffer(device, camera_buffer);
}

void update_camera(f32 delta_time)
{
    camera_controller.update(delta_time);

    BufferUpdateDesc buffer_update_desc{};
    buffer_update_desc.buffer = &camera_buffer;
    buffer_update_desc.size   = sizeof(CameraData);
    buffer_update_desc.offset = 0;
    buffer_update_desc.data   = &camera.get_data();

    update_buffer(device, buffer_update_desc);
}

void create_map_sampler()
{
    SamplerDesc sampler_desc{};
    sampler_desc.min_filter  = Filter::eNearest;
    sampler_desc.mag_filter  = Filter::eNearest;
    sampler_desc.mipmap_mode = SamplerMipmapMode::eLinear;
    sampler_desc.min_lod     = 0;
    sampler_desc.max_lod     = 1000;

    sampler = create_sampler(device, sampler_desc);
}

void destroy_map_sampler()
{
    destroy_sampler(device, sampler);
}

void create_descriptor_sets()
{
    DescriptorSetDesc descriptor_set_desc{};
    descriptor_set_desc.descriptor_set_layout = &mesh_dsl;

    scene_set = create_descriptor_set(device, descriptor_set_desc);

    DescriptorBufferDesc buffer_desc{};
    buffer_desc.buffer = &camera_buffer;
    buffer_desc.offset = 0;
    buffer_desc.range  = sizeof(CameraData);

    DescriptorImageDesc image_descs[ 1 ] = {};
    image_descs[ 0 ].sampler             = &sampler;

    DescriptorWriteDesc descriptor_write_descs[ 2 ]     = {};
    descriptor_write_descs[ 0 ].binding                 = 0;
    descriptor_write_descs[ 0 ].descriptor_type         = DescriptorType::eUniformBuffer;
    descriptor_write_descs[ 0 ].descriptor_count        = 1;
    descriptor_write_descs[ 0 ].descriptor_buffer_descs = &buffer_desc;
    descriptor_write_descs[ 1 ].binding                 = 1;
    descriptor_write_descs[ 1 ].descriptor_type         = DescriptorType::eSampler;
    descriptor_write_descs[ 1 ].descriptor_count        = 1;
    descriptor_write_descs[ 1 ].descriptor_image_descs  = &image_descs[ 0 ];

    update_descriptor_set(device, scene_set, 2, descriptor_write_descs);
}

void destroy_descriptor_sets()
{
    destroy_descriptor_set(device, scene_set);
}

void create_scene()
{
    create_mesh_pipeline();
    create_camera();
    create_map_sampler();
    create_descriptor_sets();

    endless_terrain.create(device);
}

void destroy_scene()
{
    endless_terrain.destroy();
    destroy_descriptor_sets();
    destroy_map_sampler();
    destroy_camera();
    destroy_mesh_pipeline();
}

// ui
void begin_dockspace()
{
    static bool               dockspace_open            = true;
    static bool               opt_fullscreen_persistent = true;
    bool                      opt_fullscreen            = opt_fullscreen_persistent;
    static ImGuiDockNodeFlags dockspace_flags           = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the
    // pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", &dockspace_open, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiIO&    io          = ImGui::GetIO();
    ImGuiStyle& style       = ImGui::GetStyle();
    float       minWinSizeX = style.WindowMinSize.x;
    style.WindowMinSize.x   = 100.0f;
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    style.WindowMinSize.x = minWinSizeX;
}

void end_dockspace()
{
    ImGui::End();
}

void draw_ui(CommandBuffer& cmd)
{
    ui_begin_frame();
    begin_dockspace();
    {
        ImGui::Begin("Performance");
        ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
        ImGui::End();

        ImGui::Begin("Scene");
        ImGui::Image(editor_image.image_view, ImVec2(editor_image.width, editor_image.height));
        ImGui::End();
    }
    end_dockspace();
    ui_end_frame(cmd);
}

// app
void create_editor_images(u32 width, u32 height)
{
    ImageDesc editor_image_descs[ 2 ]    = {};
    editor_image_descs[ 0 ].width        = width;
    editor_image_descs[ 0 ].height       = height;
    editor_image_descs[ 0 ].depth        = 1.0f;
    editor_image_descs[ 0 ].format       = Format::eR8G8B8A8Unorm;
    editor_image_descs[ 0 ].layer_count  = 1;
    editor_image_descs[ 0 ].mip_levels   = 1;
    editor_image_descs[ 0 ].sample_count = SampleCount::e1;
    // editor_image_descs[ 0 ].resource_state  = ResourceState::eTransferDst | ResourceState::eColorAttachment;
    editor_image_descs[ 0 ].descriptor_type = DescriptorType::eColorAttachment | DescriptorType::eSampledImage;

    editor_image_descs[ 1 ].width        = width;
    editor_image_descs[ 1 ].height       = height;
    editor_image_descs[ 1 ].depth        = 1.0f;
    editor_image_descs[ 1 ].format       = Format::eD32Sfloat;
    editor_image_descs[ 1 ].layer_count  = 1;
    editor_image_descs[ 1 ].mip_levels   = 1;
    editor_image_descs[ 1 ].sample_count = SampleCount::e1;
    // editor_image_descs[ 1 ].resource_state  = ResourceState::eTransferDst | ResourceState::eDepthStencilWrite;
    editor_image_descs[ 1 ].descriptor_type = DescriptorType::eDepthStencilAttachment;

    editor_image       = create_image(device, editor_image_descs[ 0 ]);
    editor_depth_image = create_image(device, editor_image_descs[ 1 ]);

    ImageBarrier image_barriers[ 2 ] = {};
    image_barriers[ 0 ].image        = &editor_image;
    image_barriers[ 0 ].src_queue    = &queue;
    image_barriers[ 0 ].dst_queue    = &queue;
    image_barriers[ 0 ].old_state    = ResourceState::eUndefined;
    image_barriers[ 0 ].new_state    = ResourceState::eColorAttachment;
    image_barriers[ 1 ].image        = &editor_depth_image;
    image_barriers[ 1 ].src_queue    = &queue;
    image_barriers[ 1 ].dst_queue    = &queue;
    image_barriers[ 1 ].old_state    = ResourceState::eUndefined;
    image_barriers[ 1 ].new_state    = ResourceState::eDepthStencilWrite;

    begin_command_buffer(command_buffers[ 0 ]);
    cmd_barrier(command_buffers[ 0 ], 0, nullptr, 2, image_barriers);
    end_command_buffer(command_buffers[ 0 ]);
    immediate_submit(queue, command_buffers[ 0 ]);
}

void destroy_editor_images()
{
    destroy_image(device, editor_image);
    destroy_image(device, editor_depth_image);
}

void create_editor_render_pass(u32 width, u32 height)
{
    RenderPassDesc rp_desc{};
    rp_desc.width                          = width;
    rp_desc.height                         = height;
    rp_desc.color_attachment_count         = 1;
    rp_desc.color_attachments[ 0 ]         = &editor_image;
    rp_desc.color_attachment_load_ops[ 0 ] = AttachmentLoadOp::eClear;
    rp_desc.color_image_states[ 0 ]        = ResourceState::eColorAttachment;
    rp_desc.depth_stencil                  = &editor_depth_image;
    rp_desc.depth_stencil_load_op          = AttachmentLoadOp::eClear;
    rp_desc.depth_stencil_state            = ResourceState::eDepthStencilWrite;

    editor_render_pass = create_render_pass(device, rp_desc);
}

void destroy_editor_render_pass()
{
    destroy_render_pass(device, editor_render_pass);
}

void begin_editor_render_pass(CommandBuffer& cmd)
{
    ImageBarrier to_color_attachment =
        create_image_barrier(editor_image, ResourceState::eUndefined, ResourceState::eColorAttachment, queue, queue);

    cmd_barrier(cmd, 0, nullptr, 1, &to_color_attachment);

    RenderPassBeginDesc rp_begin_desc{};
    rp_begin_desc.clear_values[ 0 ].color[ 0 ] = 0.5;
    rp_begin_desc.clear_values[ 0 ].color[ 1 ] = 0.6;
    rp_begin_desc.clear_values[ 0 ].color[ 2 ] = 0.4;
    rp_begin_desc.clear_values[ 0 ].color[ 3 ] = 1.0;
    rp_begin_desc.clear_values[ 1 ].depth      = 1.0f;
    rp_begin_desc.clear_values[ 1 ].stencil    = 0;
    rp_begin_desc.render_pass                  = &editor_render_pass;

    cmd_begin_render_pass(cmd, rp_begin_desc);
}

void end_editor_render_pass(CommandBuffer& cmd)
{
    cmd_end_render_pass(cmd);

    ImageBarrier to_sampled = create_image_barrier(
        editor_image, ResourceState::eColorAttachment, ResourceState::eShaderReadOnly, queue, queue);

    cmd_barrier(cmd, 0, nullptr, 1, &to_sampled);
}

void init_graphics()
{
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
    swapchain_desc.builtin_depth   = true;

    swapchain = create_swapchain(device, swapchain_desc);
}

void shutdown_graphics()
{
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

void init_ui()
{
    UiDesc ui_desc{};
    ui_desc.window             = get_app_window();
    ui_desc.renderer           = &renderer;
    ui_desc.device             = &device;
    ui_desc.queue              = &queue;
    ui_desc.min_image_count    = swapchain.min_image_count;
    ui_desc.image_count        = swapchain.image_count;
    ui_desc.in_fly_frame_count = FRAME_COUNT;
    ui_desc.render_pass        = &swapchain.render_passes[ 0 ];
    ui_desc.docking            = true;

    ui_context = create_ui_context(ui_desc);

    app_set_ui_context(ui_context);
}

void shutdown_ui()
{
    destroy_ui_context(device, ui_context);
}

void begin_frame(u32& image_index)
{
    if (!command_buffers_recorded[ frame_index ])
    {
        wait_for_fences(device, 1, &in_flight_fences[ frame_index ]);
        reset_fences(device, 1, &in_flight_fences[ frame_index ]);
        command_buffers_recorded[ frame_index ] = true;
    }

    acquire_next_image(device, swapchain, image_available_semaphores[ frame_index ], {}, image_index);
}

void end_frame(u32 image_index)
{
    auto& cmd = command_buffers[ frame_index ];

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

void on_init()
{
    app_set_shaders_directory("../../sandbox/data/shaders");
    app_set_textures_directory("../../sandbox/data/textures");

    init_graphics();
    init_ui();
    create_editor_images(swapchain.width, swapchain.height);
    create_editor_render_pass(swapchain.width, swapchain.height);
    create_scene();
}

void on_resize(u32 width, u32 height)
{
    queue_wait_idle(queue);
    resize_swapchain(device, swapchain, width, height);
    camera.on_resize(width, height);
}

void on_update(f32 delta_time)
{
    update_camera(delta_time);
    endless_terrain.update(camera.get_position());

    u32 image_index = 0;
    begin_frame(image_index);

    auto& cmd = command_buffers[ frame_index ];

    begin_command_buffer(cmd);
    {
        begin_editor_render_pass(cmd);

        cmd_set_viewport(cmd, 0, 0, swapchain.width, swapchain.height, 0, 1.0f);
        cmd_set_scissor(cmd, 0, 0, swapchain.width, swapchain.height);
        cmd_bind_pipeline(cmd, mesh_pipeline);
        cmd_bind_descriptor_set(cmd, scene_set, mesh_pipeline);

        for (auto& [ pos, chunk ] : endless_terrain.get_chunks())
        {
            if (chunk.is_visible())
            {
                pc.model = chunk.get_transform();
                cmd_push_constants(cmd, mesh_pipeline, 0, sizeof(pc), &pc);
                cmd_bind_vertex_buffer(cmd, chunk.get_mesh().get_vertex_buffer());
                cmd_bind_index_buffer_u32(cmd, chunk.get_mesh().get_index_buffer());
                cmd_draw_indexed(cmd, chunk.get_mesh().get_index_count(), 1, 0, 0, 0);
            }
        }
        end_editor_render_pass(cmd);

        ImageBarrier to_color_attachment = create_image_barrier(
            swapchain.images[ image_index ], ResourceState::eUndefined, ResourceState::eColorAttachment, queue, queue);

        cmd_barrier(cmd, 0, nullptr, 1, &to_color_attachment);

        RenderPassBeginDesc rp_begin_desc{};
        rp_begin_desc.clear_values[ 0 ].color[ 0 ] = 0.5;
        rp_begin_desc.clear_values[ 0 ].color[ 1 ] = 0.6;
        rp_begin_desc.clear_values[ 0 ].color[ 2 ] = 0.4;
        rp_begin_desc.clear_values[ 0 ].color[ 3 ] = 1.0;
        rp_begin_desc.clear_values[ 1 ].depth      = 1.0f;
        rp_begin_desc.clear_values[ 1 ].stencil    = 0;
        rp_begin_desc.render_pass                  = &swapchain.render_passes[ image_index ];

        cmd_begin_render_pass(cmd, rp_begin_desc);
        draw_ui(cmd);
        cmd_end_render_pass(cmd);

        ImageBarrier to_present = create_image_barrier(
            swapchain.images[ image_index ], ResourceState::eColorAttachment, ResourceState::ePresent, queue, queue);

        cmd_barrier(cmd, 0, nullptr, 1, &to_present);
    }
    end_command_buffer(cmd);
    end_frame(image_index);
}

void on_shutdown()
{
    device_wait_idle(device);
    destroy_scene();
    destroy_editor_render_pass();
    destroy_editor_images();
    shutdown_ui();
    shutdown_graphics();
}

int main(int argc, char** argv)
{
    ApplicationConfig config;
    config.argc        = argc;
    config.argv        = argv;
    config.title       = "TestApp";
    config.x           = 0;
    config.y           = 0;
    config.width       = 1400;
    config.height      = 900;
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