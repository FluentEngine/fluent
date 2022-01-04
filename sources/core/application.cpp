#include <chrono>
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
    f64 m_delta_time;
};

static ApplicationState app_state{};

std::string get_exec_path(u32 argc, char* argv_zero)
{
    std::string exec_path;
    std::string launch_directory;
    launch_directory.resize(100);
    getcwd(launch_directory.data(), 100);
    launch_directory = launch_directory.substr(0, launch_directory.find_first_of('\0'));

    // TODO: absolute path bug
    if (argv_zero[ 0 ] == '/')
    {
        launch_directory.clear();
        launch_directory = "/";
    }

    std::string launch_to_exec(argv_zero);
    launch_to_exec = launch_to_exec.substr(launch_to_exec.find_first_of('/'), launch_to_exec.find_last_of('/')) + "/";
    exec_path = launch_directory + launch_to_exec;

    return exec_path;
}

void app_set_shaders_directory(const std::string& path)
{
    auto directory = path;
    if (path.find_last_of('/') != path.length() - 1)
    {
        directory += "/";
    }

    app_state.m_shaders_directory = app_state.m_exec_path + directory;
}

void app_set_textures_directory(const std::string& path)
{
    auto directory = path;
    if (path.find_last_of('/') != path.length() - 1)
    {
        directory += "/";
    }

    app_state.m_textures_directory = app_state.m_exec_path + directory;
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

    f64 lastTick = SDL_GetTicks64();
    app_state.m_delta_time = 0.0;

    while (app_state.m_is_running)
    {
        app_state.m_delta_time = (static_cast<f64>(SDL_GetTicks64()) - lastTick);
        lastTick = SDL_GetTicks64();

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
                    app_state.m_window.m_data[ WindowParams::eWidth ] = e.window.data1;
                    app_state.m_window.m_data[ WindowParams::eHeight ] = e.window.data2;

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

} // namespace fluent
