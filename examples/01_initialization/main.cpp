#include <iostream>

#include "fluent/fluent.hpp"

using namespace fluent;

Renderer g_renderer;

void on_init()
{
    RendererDescription rendererDescription{};
    rendererDescription.vulkan_allocator = nullptr;
    g_renderer = create_renderer(rendererDescription);
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
