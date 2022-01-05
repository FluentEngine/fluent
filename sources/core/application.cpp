#include <chrono>
#include <filesystem>
#include <SDL.h>
#include "core/window.hpp"
#include "core/application.hpp"

namespace fluent
{

struct ApplicationState
{
    std::string m_exec_path;
    std::string m_shaders_directory;
    std::string m_textures_directory;
    b32 m_is_inited;
    b32 m_is_running;
    const char* m_title;
    Window m_window;
    InitCallback m_on_init;
    UpdateCallback m_on_update;
    ShutdownCallback m_on_shutdown;
    ResizeCallback m_on_resize;
    f32 m_delta_time;
};

static ApplicationState app_state{};

std::string get_exec_path(u32 argc, char* argv_zero)
{
    std::string exec_path =
        std::filesystem::weakly_canonical(std::filesystem::path(argv_zero)).parent_path().string() + "/";

    return exec_path;
}

void app_set_shaders_directory(const std::string& path)
{
    app_state.m_shaders_directory = app_state.m_exec_path + std::filesystem::path(path).string() + "/";
}

void app_set_textures_directory(const std::string& path)
{
    app_state.m_textures_directory = app_state.m_exec_path + std::filesystem::path(path).string() + "/";
}

void app_init(const ApplicationConfig* config)
{
    FT_ASSERT(config->argv);
    FT_ASSERT(config->on_init);
    FT_ASSERT(config->on_update);
    FT_ASSERT(config->on_shutdown);
    FT_ASSERT(config->on_resize);

    int init_result = SDL_Init(SDL_INIT_VIDEO);
    FT_ASSERT(init_result >= 0 && "SDL Init failed");

    app_state.m_window = fluent::create_window(config->title, config->x, config->y, config->width, config->height);

    app_state.m_exec_path = get_exec_path(config->argc, config->argv[ 0 ]);
    app_state.m_title = config->title;
    app_state.m_on_init = config->on_init;
    app_state.m_on_update = config->on_update;
    app_state.m_on_shutdown = config->on_shutdown;
    app_state.m_on_resize = config->on_resize;

    spdlog::set_level(to_spdlog_level(config->log_level));

    app_state.m_is_inited = true;
}

void app_run()
{
    FT_ASSERT(app_state.m_is_inited);

    app_state.m_on_init();

    app_state.m_is_running = true;

    SDL_Event e;

    f32 last_tick = get_time();
    app_state.m_delta_time = 0.0;

    while (app_state.m_is_running)
    {
        app_state.m_delta_time = (get_time() - last_tick);
        last_tick = get_time();

        while (SDL_PollEvent(&e) != 0)
        {
            switch (e.type)
            {
            case SDL_QUIT:
                app_state.m_is_running = false;
                break;
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    app_state.m_window.data[ WindowParams::eWidth ] = e.window.data1;
                    app_state.m_window.data[ WindowParams::eHeight ] = e.window.data2;

                    app_state.m_on_resize(e.window.data1, e.window.data2);
                }
                break;
            default:
                break;
            }
        }

        app_state.m_on_update(app_state.m_delta_time);
    }

    app_state.m_on_shutdown();
}

void app_shutdown()
{
    FT_ASSERT(app_state.m_is_inited);
    fluent::destroy_window(app_state.m_window);
    SDL_Quit();
}

const char* get_app_name()
{
    return app_state.m_title;
}

const Window* get_app_window()
{
    return &app_state.m_window;
}

const std::string& get_app_shaders_directory()
{
    return app_state.m_shaders_directory;
}

const std::string& get_app_textures_directory()
{
    return app_state.m_textures_directory;
}

f32 get_time()
{
    return static_cast<f32>(SDL_GetTicks64());
}

f32 get_delta_time()
{
    return app_state.m_delta_time;
}

} // namespace fluent
