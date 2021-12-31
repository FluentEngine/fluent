#include <iostream>

#include "fluent/fluent.hpp"

using namespace fluent;

void on_load(u32 width, u32 height)
{
}

void on_update(f64 deltaTime)
{
    FT_INFO("Delta time {}", deltaTime);
}

void on_render()
{
}

void on_unload()
{
}

int main()
{
    ApplicationConfig config;
    config.title = "TestApp";
    config.x = 0;
    config.y = 0;
    config.width = 800;
    config.height = 600;
    config.on_load = on_load;
    config.on_update = on_update;
    config.on_render = on_render;
    config.on_unload = on_unload;

    application_init(&config);
    application_run();
    application_shutdown();

    return 0;
}
