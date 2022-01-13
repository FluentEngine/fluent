#include <functional>
#include <random>
#include "fluent/fluent.hpp"

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
UiContext                                            ui_context;
std::vector<std::function<void(CommandBuffer& cmd)>> deffered_updates;

// scene
struct
{
    u32     width       = 100;
    u32     height      = 100;
    f32     scale       = 27.6f;
    i32     octaves     = 4;
    f32     persistance = 0.5f;
    f32     lacunarity  = 2.0f;
    i32     seed        = 0.0f;
    Vector2 offset      = Vector2(0.0f, 0.0f);
} noise_settings;

Image noise_texture;

// helpers
ImageBarrier create_image_barrier(
    Image& image, ResourceState old_state, ResourceState new_state, Queue& src_queue, Queue& dst_queue);

// common functions
void init_graphics();
void shutdown_graphics();
void init_ui();
void shutdown_ui();

void begin_frame(u32& image_index);
void end_frame(u32 image_index);

// scene functions
std::vector<u8> generate_noise_texture(
    f32 width, f32 height, f32 scale, u32 octaves, f32 persistance, f32 lacunarity, int seed, Vector2 offset);
Image create_noise_texture(
    f32 width, f32 height, f32 scale, u32 octaves, f32 persistance, f32 lacunarity, int seed, Vector2 offset);
void draw_noise_settings();
void update_noise_texture(CommandBuffer& cmd);
void create_scene();
void destroy_scene();

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
        ui_begin_frame();
        ImGui::Begin("Noise texture");
        ImGui::Image(noise_texture.image_view, ImVec2(512, 512));
        ImGui::End();
        draw_noise_settings();
        ui_end_frame(cmd);
        cmd_end_render_pass(cmd);

        ImageBarrier to_present = create_image_barrier(
            swapchain.images[ image_index ], ResourceState::eColorAttachment, ResourceState::ePresent, queue, queue);

        cmd_barrier(cmd, 0, nullptr, 1, &to_present);

        for (auto& update : deffered_updates)
        {
            update(cmd);
        }
        deffered_updates.clear();
    }
    end_command_buffer(cmd);
    end_frame(image_index);
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

std::vector<u8> generate_noise_texture(
    f32 width, f32 height, f32 scale, u32 octaves, f32 persistance, f32 lacunarity, int seed, Vector2 offset)
{
    std::mt19937                        eng(seed);
    std::uniform_real_distribution<f32> urd(-100000, 100000);

    std::vector<Vector2> octave_offsets(octaves);
    for (u32 i = 0; i < octaves; ++i)
    {
        octave_offsets[ i ].x = urd(eng) + offset.x;
        octave_offsets[ i ].y = urd(eng) + offset.y;
    }

    static constexpr u32 bpp = 4;
    std::vector<f32>     image_data(width * height * bpp);

    f32 min_noise_height = std::numeric_limits<f32>::max();
    f32 max_noise_height = std::numeric_limits<f32>::min();

    i32 half_width  = width / 2;
    i32 half_height = height / 2;

    for (i32 y = 0; y < height; ++y)
    {
        for (i32 x = 0; x < width; ++x)
        {
            f32 amplitude    = 1.0f;
            f32 frequency    = 1.0f;
            f32 noise_height = 0.0f;

            for (u32 i = 0; i < octaves; ++i)
            {
                f32 sample_x = ( f32 ) (x - half_width) / scale * frequency + octave_offsets[ i ].x;
                f32 sample_y = ( f32 ) (y - half_height) / scale * frequency + octave_offsets[ i ].y;

                f32 perlin_value = perlin_noise_2d(Vector2(sample_x, sample_y)) * 2.0 - 1.0;

                noise_height += perlin_value * amplitude;

                amplitude *= persistance;
                frequency *= lacunarity;
            }

            if (noise_height > max_noise_height)
            {
                max_noise_height = noise_height;
            }
            else if (noise_height < min_noise_height)
            {
                min_noise_height = noise_height;
            }

            u32 idx           = x * bpp + y * bpp * width;
            image_data[ idx ] = noise_height;
        }
    }

    std::vector<u8> result(image_data.size());

    for (u32 y = 0; y < height; ++y)
    {
        for (u32 x = 0; x < width; ++x)
        {
            u32 idx           = x * bpp + y * bpp * width;
            result[ idx ]     = inverse_lerp(min_noise_height, max_noise_height, image_data[ idx ]) * 255.0f;
            result[ idx + 1 ] = result[ idx ];
            result[ idx + 2 ] = result[ idx ];
            result[ idx + 3 ] = 255;
        }
    }

    return result;
}

Image create_noise_texture(
    f32 width, f32 height, f32 scale, u32 octaves, f32 persistance, f32 lacunarity, int seed, Vector2 offset)
{
    static constexpr u32 noise_texture_size = 100;
    static constexpr u32 bpp                = 4;

    auto image_data = generate_noise_texture(width, height, scale, octaves, persistance, lacunarity, seed, offset);

    ImageDesc desc{};
    desc.width           = noise_texture_size;
    desc.height          = noise_texture_size;
    desc.depth           = 1;
    desc.sample_count    = SampleCount::e1;
    desc.mip_levels      = 1;
    desc.layer_count     = 1;
    desc.format          = Format::eR8G8B8A8Unorm;
    desc.descriptor_type = DescriptorType::eSampledImage;
    desc.resource_state  = ResourceState::eTransferDst | ResourceState::eShaderReadOnly;
    desc.data_size       = noise_texture_size * noise_texture_size * bpp;
    desc.data            = image_data.data();

    return create_image(device, desc);
}

void update_noise_texture(CommandBuffer& cmd)
{
    auto image_data = generate_noise_texture(
        noise_settings.width, noise_settings.height, noise_settings.scale, noise_settings.octaves,
        noise_settings.persistance, noise_settings.lacunarity, noise_settings.seed, noise_settings.offset);
    ImageUpdateDesc desc{};
    desc.data           = image_data.data();
    desc.size           = image_data.size() * sizeof(image_data[ 0 ]);
    desc.image          = &noise_texture;
    desc.resource_state = ResourceState::eShaderReadOnly;
    update_image(device, desc);
}

void draw_noise_settings()
{
    ImGui::Begin("Noise settings");
    bool need_update = false;

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
        deffered_updates.push_back(update_noise_texture);
    }
    ImGui::End();
}

void create_scene()
{
    noise_texture = create_noise_texture(
        noise_settings.width, noise_settings.height, noise_settings.scale, noise_settings.octaves,
        noise_settings.persistance, noise_settings.lacunarity, noise_settings.seed, noise_settings.offset);
}

void destroy_scene()
{
    destroy_image(device, noise_texture);
}