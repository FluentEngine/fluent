#include <iostream>

#include "fluent/fluent.hpp"

using namespace fluent;

Camera           camera;
CameraController camera_controller;
InputSystem*     input_system;
UiContext*       ui_context;

Ref<Geometry> cube       = nullptr;
Ref<Geometry> plane      = nullptr;
Ref<Image>    cyan       = nullptr;
Ref<Image>    light_gray = nullptr;

struct
{
    u32 scene_viewport_width   = 1400;
    u32 scene_viewport_height  = 900;
    b32 scene_viewport_changed = false;
} ui_data;

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
    f32         minWinSizeX = style.WindowMinSize.x;
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

void draw_ui(CommandBuffer* cmd)
{
    ui_begin_frame();
    begin_dockspace();
    {
        ImGui::Begin("Performance");
        ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
        ImGui::End();

        ImGui::Begin("Scene");
        ImVec2 viewport_size = ImGui::GetContentRegionAvail();
        if (viewport_size.x != ui_data.scene_viewport_width || viewport_size.y != ui_data.scene_viewport_height)
        {
            ui_data.scene_viewport_width   = viewport_size.x;
            ui_data.scene_viewport_height  = viewport_size.y;
            ui_data.scene_viewport_changed = true;
        }
        ImGui::Image(
            Renderer3D::get_output_image()->image_view,
            ImVec2(ui_data.scene_viewport_width, ui_data.scene_viewport_height));
        ImGui::End();
    }
    end_dockspace();
    ui_end_frame(cmd);
}

void on_init()
{
    app_set_shaders_directory("../../sandbox/shaders/");
    app_set_models_directory("../../sandbox/models/");
    app_set_textures_directory("../../sandbox/textures/");

    GraphicContext::init();
    ResourceManager::init(GraphicContext::get()->device());
    Renderer3D::init(GraphicContext::get()->swapchain()->width, GraphicContext::get()->swapchain()->height);

    UiDesc ui_desc{};
    ui_desc.window             = get_app_window();
    ui_desc.renderer           = GraphicContext::get()->backend();
    ui_desc.device             = GraphicContext::get()->device();
    ui_desc.queue              = GraphicContext::get()->queue();
    ui_desc.min_image_count    = GraphicContext::get()->swapchain()->min_image_count;
    ui_desc.image_count        = GraphicContext::get()->swapchain()->image_count;
    ui_desc.in_fly_frame_count = 2; // TODO:
    ui_desc.render_pass        = GraphicContext::get()->swapchain()->render_passes[ 0 ];
    ui_desc.docking            = true;

    create_ui_context(&ui_desc, &ui_context);

    GeometryLoadDesc geom_load_desc{};
    geom_load_desc.filename        = "cube.gltf";
    geom_load_desc.load_normals    = true;
    geom_load_desc.load_tex_coords = true;
    geom_load_desc.load_tangents   = true;
    geom_load_desc.load_bitangents = true;
    ResourceManager::load_geometry(cube, &geom_load_desc);
    geom_load_desc.filename = "plane.gltf";
    ResourceManager::load_geometry(plane, &geom_load_desc);

    camera.init_camera(Vector3(0.0f, 1.0, 3.0f), Vector3(0.0, 0.0, -1.0), Vector3(0.0, 1.0, 0.0));
    camera_controller.init(get_app_input_system(), camera);

    u32 image_width  = 2;
    u32 image_height = 2;

    struct Color
    {
        u8 data[ 4 ];
    };

    Color cyan_color       = { 0, 255, 255, 255 };
    Color light_gray_color = { 120, 120, 120, 255 };

    std::vector<Color> cyan_image_data(image_width * image_height, cyan_color);
    std::vector<Color> light_gray_image_data(image_width * image_height, light_gray_color);

    ImageLoadDesc image_load_desc{};
    image_load_desc.size       = cyan_image_data.size() * sizeof(cyan_image_data[ 0 ]);
    image_load_desc.data       = cyan_image_data.data();
    ImageDesc& image_desc      = image_load_desc.image_desc;
    image_desc.width           = image_width;
    image_desc.height          = image_height;
    image_desc.depth           = 1;
    image_desc.format          = Format::eR8G8B8A8Unorm;
    image_desc.layer_count     = 1;
    image_desc.mip_levels      = 1;
    image_desc.sample_count    = SampleCount::e1;
    image_desc.descriptor_type = DescriptorType::eSampledImage;

    ResourceManager::load_image(cyan, &image_load_desc);

    image_load_desc.size = light_gray_image_data.size() * sizeof(light_gray_image_data[ 0 ]);
    image_load_desc.data = light_gray_image_data.data();

    ResourceManager::load_image(light_gray, &image_load_desc);
}

void on_resize(u32 width, u32 height)
{
    GraphicContext::get()->on_resize(width, height);
}

void on_update(f32 delta_time)
{
    camera_controller.update(delta_time);

    if (ui_data.scene_viewport_changed)
    {
        ui_data.scene_viewport_changed = false;
        queue_wait_idle(GraphicContext::get()->queue());
        Renderer3D::resize_viewport(ui_data.scene_viewport_width, ui_data.scene_viewport_height);
        camera.on_resize(ui_data.scene_viewport_width, ui_data.scene_viewport_height);
    }

    GraphicContext::get()->begin_frame();

    auto* cmd = GraphicContext::get()->acquire_cmd();

    begin_command_buffer(cmd);

    Renderer3D::begin_frame(camera);
    Renderer3D::draw_geometry(Matrix4(1.0f), cube, cyan);
    Renderer3D::draw_geometry(translate(Matrix4(1.0f), Vector3(5.0, 0.0, 0.0)), cube);
    Renderer3D::draw_geometry(
        scale(translate(Matrix4(1.0f), Vector3(0.0, -1.0, 0.0)), Vector3(60.0f)), plane, light_gray);
    Renderer3D::end_frame();

    ImageBarrier to_color_attachment{};
    to_color_attachment.src_queue = GraphicContext::get()->queue();
    to_color_attachment.dst_queue = GraphicContext::get()->queue();
    to_color_attachment.image     = GraphicContext::get()->acquire_image();
    to_color_attachment.old_state = ResourceState::eUndefined;
    to_color_attachment.new_state = ResourceState::eColorAttachment;

    cmd_barrier(cmd, 0, nullptr, 1, &to_color_attachment);

    RenderPassBeginDesc render_pass_begin_desc{};
    render_pass_begin_desc.render_pass                  = GraphicContext::get()->acquire_render_pass();
    render_pass_begin_desc.clear_values[ 0 ].color[ 0 ] = 1.0f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 1 ] = 0.8f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 2 ] = 0.4f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 3 ] = 1.0f;

    cmd_begin_render_pass(cmd, &render_pass_begin_desc);
    draw_ui(cmd);
    cmd_end_render_pass(cmd);

    ImageBarrier to_present_barrier{};
    to_present_barrier.src_queue = GraphicContext::get()->queue();
    to_present_barrier.dst_queue = GraphicContext::get()->queue();
    to_present_barrier.image     = GraphicContext::get()->acquire_image();
    to_present_barrier.old_state = ResourceState::eUndefined;
    to_present_barrier.new_state = ResourceState::ePresent;

    cmd_barrier(cmd, 0, nullptr, 1, &to_present_barrier);

    end_command_buffer(cmd);

    GraphicContext::get()->end_frame();
}

void on_shutdown()
{
    device_wait_idle(GraphicContext::get()->device());
    ResourceManager::release_geometry(cube);
    ResourceManager::release_geometry(plane);
    ResourceManager::release_image(cyan);
    ResourceManager::release_image(light_gray);
    destroy_ui_context(GraphicContext::get()->device(), ui_context);
    Renderer3D::shutdown();
    ResourceManager::shutdown();
    GraphicContext::shutdown();
}

int main(int argc, char** argv)
{
    ApplicationConfig config;
    config.argc        = argc;
    config.argv        = argv;
    config.title       = "Sandbox";
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
