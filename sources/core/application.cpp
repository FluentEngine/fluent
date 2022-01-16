#include <chrono>
#include <filesystem>
#include <SDL.h>
#include "imgui_impl_sdl.h"
#include "core/window.hpp"
#include "core/application.hpp"
#include "core/input.hpp"

namespace fluent
{

using ImGuiCallback = bool (*)(const SDL_Event* e);

struct ApplicationState
{
    std::string      exec_path;
    std::string      shaders_directory;
    std::string      textures_directory;
    std::string      models_directory;
    b32              is_inited;
    b32              is_running;
    const char*      title;
    Window           window;
    InitCallback     on_init;
    UpdateCallback   on_update;
    ShutdownCallback on_shutdown;
    ResizeCallback   on_resize;
    f32              delta_time;
    InputSystem      input_system;
    ImGuiCallback    imgui_callback;
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
    app_state.shaders_directory = app_state.exec_path + std::filesystem::path(path).string() + "/";
}

void app_set_textures_directory(const std::string& path)
{
    app_state.textures_directory = app_state.exec_path + std::filesystem::path(path).string() + "/";
}

void app_set_models_directory(const std::string& path)
{
    app_state.models_directory = app_state.exec_path + std::filesystem::path(path).string() + "/";
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

    app_state.window = fluent::create_window(config->title, config->x, config->y, config->width, config->height);

    app_state.exec_path      = get_exec_path(config->argc, config->argv[ 0 ]);
    app_state.title          = config->title;
    app_state.on_init        = config->on_init;
    app_state.on_update      = config->on_update;
    app_state.on_shutdown    = config->on_shutdown;
    app_state.on_resize      = config->on_resize;
    app_state.imgui_callback = [](const SDL_Event* e) -> bool { return false; };
    spdlog::set_level(to_spdlog_level(config->log_level));

    init_input_system(&app_state.input_system);

    app_state.is_inited = true;
}

void app_run()
{
    FT_ASSERT(app_state.is_inited);

    app_state.on_init();

    app_state.is_running = true;

    SDL_Event e;

    f32 last_tick        = get_time();
    app_state.delta_time = 0.0;

    while (app_state.is_running)
    {
        app_state.delta_time = (get_time() - last_tick) / 60;
        last_tick            = get_time();
        update_input_system(&app_state.input_system);

        while (SDL_PollEvent(&e) != 0)
        {
            app_state.imgui_callback(&e);
            switch (e.type)
            {
            case SDL_QUIT:
                app_state.is_running = false;
                break;
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    app_state.window.data[ WindowParams::eWidth ]  = e.window.data1;
                    app_state.window.data[ WindowParams::eHeight ] = e.window.data2;

                    app_state.on_resize(e.window.data1, e.window.data2);
                }
                break;
            case SDL_KEYDOWN:
                update_input_keyboard_state(&app_state.input_system, static_cast<KeyCode>(e.key.keysym.scancode), true);
                break;
            case SDL_KEYUP:
                update_input_keyboard_state(
                    &app_state.input_system, static_cast<KeyCode>(e.key.keysym.scancode), false);
                break;
            case SDL_MOUSEMOTION:
                update_input_mouse_state(&app_state.input_system, e.motion.x, e.motion.y);
                break;
            default:
                break;
            }
        }
        app_state.on_update(app_state.delta_time);
    }

    app_state.on_shutdown();
}

void app_shutdown()
{
    FT_ASSERT(app_state.is_inited);
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

const InputSystem* get_app_input_system()
{
    return &app_state.input_system;
}

const std::string& get_app_shaders_directory()
{
    return app_state.shaders_directory;
}

const std::string& get_app_textures_directory()
{
    return app_state.textures_directory;
}

const std::string& get_app_models_directory()
{
    return app_state.models_directory;
}

void app_set_ui_context(const UiContext& context)
{
    // Bad but ok for now
    app_state.imgui_callback = ImGui_ImplSDL2_ProcessEvent;
}

f32 get_time()
{
    return static_cast<f32>(SDL_GetTicks64());
}

f32 get_delta_time()
{
    return app_state.delta_time;
}

} // namespace fluent
