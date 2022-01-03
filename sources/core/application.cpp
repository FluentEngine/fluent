#include <chrono>
#include <SDL.h>
#include "core/window.hpp"
#include "core/application.hpp"

namespace fluent
{

struct ApplicationState
{
    bool is_running;
    const char* title;
    Window window;

    InitCallback on_init;
    UpdateCallback on_update;
    ShutdownCallback on_shutdown;
    ResizeCallback on_resize;

    f64 delta_time;
};

static ApplicationState app_state{};

void application_init(const ApplicationConfig* config)
{
    FT_ASSERT(config->on_init);
    FT_ASSERT(config->on_update);
    FT_ASSERT(config->on_shutdown);
    FT_ASSERT(config->on_resize);

    int init_result = SDL_Init(SDL_INIT_VIDEO);
    FT_ASSERT(init_result >= 0 && "SDL Init failed");

    app_state.window = fluent::create_window(config->title, config->x, config->y, config->width, config->height);

    app_state.title = config->title;
    app_state.on_init = config->on_init;
    app_state.on_update = config->on_update;
    app_state.on_shutdown = config->on_shutdown;
    app_state.on_resize = config->on_resize;

    spdlog::set_level(to_spdlog_level(config->log_level));
}

void application_run()
{
    app_state.on_init();

    app_state.is_running = true;

    SDL_Event e;

    f64 lastTick = SDL_GetTicks64();
    app_state.delta_time = 0.0;

    while (app_state.is_running)
    {
        app_state.delta_time = (static_cast<f64>(SDL_GetTicks64()) - lastTick);
        lastTick = SDL_GetTicks64();

        while (SDL_PollEvent(&e) != 0)
        {
            switch (e.type)
            {
            case SDL_QUIT:
                app_state.is_running = false;
                break;
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    app_state.window.m_data[ WindowParams::eWidth ] = e.window.data1;
                    app_state.window.m_data[ WindowParams::eHeight ] = e.window.data2;

                    app_state.on_resize(e.window.data1, e.window.data2);
                }
                break;
            default:
                break;
            }
        }

        app_state.on_update(app_state.delta_time);
    }

    app_state.on_shutdown();
}

void application_shutdown()
{
    fluent::destroy_window(app_state.window);
    SDL_Quit();
}

const char* get_app_name()
{
    return app_state.title;
}

const Window* get_app_window()
{
    return &app_state.window;
}

} // namespace fluent
