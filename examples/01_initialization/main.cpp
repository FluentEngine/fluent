#include <iostream>

#include "fluent/fluent.hpp"

using namespace fluent;

Renderer g_renderer;
Device g_device;
Queue g_queue;

void on_init()
{
    RendererDescription renderer_description{};
    renderer_description.vulkan_allocator = nullptr;
    g_renderer = create_renderer(renderer_description);

    DeviceDescription device_description{};
    g_device = create_device(g_renderer, device_description);

    QueueDescription queue_description{};
    queue_description.queue_type = QueueType::eGraphics;
    g_queue = get_queue(g_renderer, g_device, queue_description);
}

void on_load(u32 width, u32 height)
{
}

void on_update(f64 deltaTime)
{
}

void on_render()
{
}

void on_unload()
{
}

void on_shutdown()
{
    destroy_device(g_renderer, g_device);
    destroy_renderer(g_renderer);
}

int main()
{
    ApplicationConfig config;
    config.title = "TestApp";
    config.x = 0;
    config.y = 0;
    config.width = 800;
    config.height = 600;
    config.log_level = LogLevel::eTrace;
    config.on_init = on_init;
    config.on_load = on_load;
    config.on_update = on_update;
    config.on_render = on_render;
    config.on_unload = on_unload;
    config.on_shutdown = on_shutdown;

    application_init(&config);
    application_run();
    application_shutdown();

    return 0;
}
