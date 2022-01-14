#include <functional>
#include <chrono>
#include <thread>
#include "fluent/fluent.hpp"
#include "noise.hpp"
#include "map_generator.hpp"

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

// scene
static constexpr u32 MAP_SIZE = 100;
NoiseSettings        noise_settings;
TerrainTypes         terrain_types;
MapGenerator         map_generator;

enum class MapType : u32
{
    eNoise   = 0,
    eTerrain = 1,
    eLast
};

static i32 selected_map = static_cast<i32>(MapType::eTerrain);

Image maps[ 2 ];
struct MapTextureUpdate
{
    const Map* map;
    Image*     texture;
};

std::vector<MapTextureUpdate> map_texture_updates;

// helpers
ImageBarrier create_image_barrier(
    Image& image, ResourceState old_state, ResourceState new_state, Queue& src_queue, Queue& dst_queue);

// common functions
void init_graphics();
void shutdown_graphics();
void init_ui();
void shutdown_ui();
void draw_ui(CommandBuffer& cmd);

void begin_frame(u32& image_index);
void end_frame(u32 image_index);

// scene functions
Image create_texture_from_map(const Map& map);
void  update_map_texture(CommandBuffer& cmd, const Map& map, Image& texture);
void  draw_noise_settings();
void  draw_terrain_settings();
void  create_scene();
void  destroy_scene();

void on_init()
{
    app_set_shaders_directory("../../../sandbox/data/shaders/");
    app_set_shaders_directory("../../../sandbox/data/textures/");

    init_graphics();
    init_ui();
    create_scene();
}

void on_resize(u32 width, u32 height)
{
    queue_wait_idle(queue);
    destroy_swapchain(device, swapchain);

    SwapchainDesc swapchain_desc{};
    swapchain_desc.width       = window_get_width(get_app_window());
    swapchain_desc.height      = window_get_height(get_app_window());
    swapchain_desc.queue       = &queue;
    swapchain_desc.image_count = FRAME_COUNT;

    swapchain = create_swapchain(renderer, device, swapchain_desc);
}

void on_update(f64 delta_time)
{
    u32 image_index = 0;
    begin_frame(image_index);

    auto& cmd = command_buffers[ frame_index ];

    begin_command_buffer(cmd);
    {
        ImageBarrier to_color_attachment = create_image_barrier(
            swapchain.images[ image_index ], ResourceState::eUndefined, ResourceState::eColorAttachment, queue, queue);

        cmd_barrier(cmd, 0, nullptr, 1, &to_color_attachment);

        RenderPassBeginDesc rp_begin_desc{};
        rp_begin_desc.clear_values[ 0 ].color[ 0 ] = 0.5;
        rp_begin_desc.clear_values[ 0 ].color[ 1 ] = 0.6;
        rp_begin_desc.clear_values[ 0 ].color[ 2 ] = 0.4;
        rp_begin_desc.clear_values[ 0 ].color[ 3 ] = 1.0;
        rp_begin_desc.render_pass                  = &swapchain.render_passes[ image_index ];

        cmd_begin_render_pass(cmd, rp_begin_desc);
        draw_ui(cmd);
        cmd_end_render_pass(cmd);

        ImageBarrier to_present = create_image_barrier(
            swapchain.images[ image_index ], ResourceState::eColorAttachment, ResourceState::ePresent, queue, queue);

        cmd_barrier(cmd, 0, nullptr, 1, &to_present);

        for (auto& update : map_texture_updates)
        {
            update_map_texture(cmd, *update.map, *update.texture);
        }

        map_texture_updates.clear();
    }
    end_command_buffer(cmd);
    end_frame(image_index);

    std::this_thread::sleep_for(std::chrono::milliseconds(( u64 ) (( f32 ) ImGui::GetIO().Framerate / 100.0f)));
}

void on_shutdown()
{
    device_wait_idle(device);
    destroy_scene();
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
    swapchain_desc.width       = window_get_width(get_app_window());
    swapchain_desc.height      = window_get_height(get_app_window());
    swapchain_desc.queue       = &queue;
    swapchain_desc.image_count = FRAME_COUNT;

    swapchain = create_swapchain(renderer, device, swapchain_desc);
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
    ui_desc.window      = get_app_window();
    ui_desc.renderer    = &renderer;
    ui_desc.device      = &device;
    ui_desc.queue       = &queue;
    ui_desc.image_count = FRAME_COUNT;
    ui_desc.render_pass = &swapchain.render_passes[ 0 ];
    ui_desc.docking     = true;

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

Image create_texture_from_map(const Map& map)
{
    static constexpr u32 bpp = 4;
    ImageDesc            desc{};
    desc.width           = MAP_SIZE;
    desc.height          = MAP_SIZE;
    desc.depth           = 1;
    desc.sample_count    = SampleCount::e1;
    desc.mip_levels      = 1;
    desc.layer_count     = 1;
    desc.format          = Format::eR8G8B8A8Unorm;
    desc.descriptor_type = DescriptorType::eSampledImage;
    desc.resource_state  = ResourceState::eTransferDst | ResourceState::eShaderReadOnly;
    desc.data_size       = map.size() * sizeof(map[ 0 ]);
    desc.data            = map.data();

    return create_image(device, desc);
}

void update_map_texture(CommandBuffer& cmd, const Map& map, Image& texture)
{
    ImageUpdateDesc desc{};
    desc.data           = map.data();
    desc.size           = map.size() * sizeof(map[ 0 ]);
    desc.image          = &texture;
    desc.resource_state = ResourceState::eShaderReadOnly;
    update_image(device, desc);
}

void draw_noise_settings()
{
    b32 need_update = false;

    if (ImGui::SliderFloat("Scale", &noise_settings.scale, 0.001, 100.0f, nullptr, 0))
    {
        need_update = true;
    }

    if (ImGui::SliderInt("Octaves", &noise_settings.octaves, 0, 32, nullptr, 0))
    {
        need_update = true;
    }

    if (ImGui::SliderFloat("Persistance", &noise_settings.persistance, 0.0f, 15.0f, nullptr, 0))
    {
        need_update = true;
    }

    if (ImGui::SliderFloat("Lacunarity", &noise_settings.lacunarity, 0.0f, 15.0f, nullptr, 0))
    {
        need_update = true;
    }

    if (ImGui::DragInt("Seed", &noise_settings.seed))
    {
        need_update = true;
    }

    if (ImGui::SliderFloat2("Offset", &noise_settings.offset.x, -20.0f, 20.0f, nullptr, 0))
    {
        need_update = true;
    }

    if (need_update)
    {
        map_generator.update_noise_map(noise_settings);
        map_generator.update_terrain_map(terrain_types);
        map_texture_updates.push_back(
            MapTextureUpdate{ &map_generator.get_noise_map(), &maps[ ( u32 ) MapType::eNoise ] });
        map_texture_updates.push_back({ &map_generator.get_terrain_map(), &maps[ ( u32 ) MapType::eTerrain ] });
    }
}

void draw_terrain_settings()
{
    for (u32 i = 0; i < terrain_types.size(); ++i)
    {
        std::string label        = "Terrain type " + std::to_string(i);
        std::string height_label = "##Height" + std::to_string(i);
        ImGui::ColorEdit3(label.c_str(), &terrain_types[ i ].color.r, ImGuiColorEditFlags_NoInputs);
        ImGui::SliderFloat(height_label.c_str(), &terrain_types[ i ].height, 0, 255.0f, nullptr, 0);
    }

    if (ImGui::Button("Apply settings"))
    {
        map_generator.update_terrain_map(terrain_types);
        map_texture_updates.push_back(
            MapTextureUpdate{ &map_generator.get_terrain_map(), &maps[ ( u32 ) MapType::eTerrain ] });
    }
}

void create_scene()
{
    terrain_types.push_back({ 23.0f, Vector3(0.214, 0.751, 0.925) });  // water
    terrain_types.push_back({ 57.0f, Vector3(0.966, 0.965, 0.613) });  // sand
    terrain_types.push_back({ 86.0f, Vector3(0.331, 1.0, 0.342) });    // ground
    terrain_types.push_back({ 159.0f, Vector3(0.225, 0.225, 0.217) }); // mountains
    terrain_types.push_back({ 210.0f, Vector3(1.0, 1.0, 1.0) });       // snow

    map_generator.set_map_size(MAP_SIZE, MAP_SIZE);
    map_generator.update_noise_map(noise_settings);
    map_generator.update_terrain_map(terrain_types);

    maps[ ( u32 ) MapType::eNoise ]   = create_texture_from_map(map_generator.get_noise_map());
    maps[ ( u32 ) MapType::eTerrain ] = create_texture_from_map(map_generator.get_terrain_map());
}

void destroy_scene()
{
    for (u32 i = 0; i < ( u32 ) MapType::eLast; ++i)
    {
        destroy_image(device, maps[ i ]);
    }
}

// ---------------------------------- UI ----------------------------=== //

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
}

void draw_ui(CommandBuffer& cmd)
{
    ui_begin_frame();

    begin_dockspace();
    {
        ImGui::Begin("Performance");
        ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
        ImGui::End();

        ImGui::Begin("Noise texture");
        ImGui::Image(maps[ selected_map ].image_view, ImVec2(512, 512));
        ImGui::End();
        ImGui::Begin("Maps settings");
        ImGui::SetNextItemOpen(true);
        if (ImGui::TreeNode("Render map"))
        {
            ImGui::RadioButton("Noise map", &selected_map, 0);
            ImGui::RadioButton("Terrain map", &selected_map, 1);
            ImGui::TreePop();
        }
        ImGui::SetNextItemOpen(true);
        if (ImGui::TreeNode("Noise settings"))
        {
            draw_noise_settings();
            ImGui::TreePop();
        }
        ImGui::SetNextItemOpen(true);
        if (ImGui::TreeNode("Terrain settings"))
        {
            draw_terrain_settings();
            ImGui::TreePop();
        }
        ImGui::End();

        ImGui::End();
    }
    end_dockspace();
    ui_end_frame(cmd);
}