#include <chrono>
#include <SDL.h>
#include "core/window.hpp"
#include "core/application.hpp"

namespace fluent
{

struct ApplicationState
{
    bool is_running;
    Window window;
    LoadCallback on_load;
    UpdateCallback on_update;
    RenderCallback on_render;
    UnloadCallback on_unload;

    f64 delta_time;
};

static ApplicationState app_state{};

void application_init(const ApplicationConfig* config)
{
    FT_ASSERT(config->on_load);
    FT_ASSERT(config->on_update);
    FT_ASSERT(config->on_render);
    FT_ASSERT(config->on_unload);

    FT_ASSERT(SDL_Init(SDL_INIT_VIDEO) >= 0);

    app_state.window = fluent::create_window(config->title, config->x, config->y, config->width, config->height);

    app_state.on_load = config->on_load;
    app_state.on_update = config->on_update;
    app_state.on_render = config->on_render;
    app_state.on_unload = config->on_unload;
}

void application_run()
{
    app_state.is_running = true;

    SDL_Event e;

    f64 lastTick = SDL_GetTicks64();
    app_state.delta_time = 0.0;

    // main loop
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

                    app_state.on_unload();
                    app_state.on_load(
                        app_state.window.m_data[ WindowParams::eWidth ],
                        app_state.window.m_data[ WindowParams::eHeight ]);
                }
                break;
            default:
                break;
            }
        }

        app_state.on_update(app_state.delta_time);
        app_state.on_render();
    }
}

void application_shutdown()
{
    fluent::destroy_window(app_state.window);
    SDL_Quit();
}

} // namespace fluent
