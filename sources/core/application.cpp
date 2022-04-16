#include <chrono>
#include <filesystem>
#include <SDL.h>
#include <imgui_impl_sdl.h>
#include "core/window.hpp"
#include "core/application.hpp"
#include "core/input.hpp"
#include "fs/file_system.hpp"

namespace fluent
{
using ImGuiCallback = bool ( * )( const SDL_Event* e );

struct ApplicationState
{
    b32              is_inited;
    b32              is_running;
    Window           window;
    InitCallback     on_init;
    UpdateCallback   on_update;
    ShutdownCallback on_shutdown;
    ResizeCallback   on_resize;
    f32              delta_time;
    InputSystem      input_system;
    ImGuiCallback    imgui_callback;
};

static ApplicationState app_state {};

void app_init( const ApplicationConfig* config )
{
    FT_ASSERT( config->argv );
    FT_ASSERT( config->on_init );
    FT_ASSERT( config->on_update );
    FT_ASSERT( config->on_shutdown );
    FT_ASSERT( config->on_resize );

    app_state.window = fluent::create_window( config->window_desc );

    app_state.on_init        = config->on_init;
    app_state.on_update      = config->on_update;
    app_state.on_shutdown    = config->on_shutdown;
    app_state.on_resize      = config->on_resize;
	app_state.imgui_callback = []( const SDL_Event* ) -> bool { return false; };
    spdlog::set_level( to_spdlog_level( config->log_level ) );

    init_input_system( &app_state.input_system );

    fs::init( config->argv );

    app_state.is_inited = true;
}

void app_run()
{
    FT_ASSERT( app_state.is_inited );

    app_state.on_init();

    app_state.is_running = true;

    SDL_Event e;

    u32 last_frame       = 0.0f;
    app_state.delta_time = 0.0;

    while ( app_state.is_running )
    {
        update_input_system( &app_state.input_system );

        u32 current_frame    = get_time();
        app_state.delta_time = ( current_frame - last_frame ) / 1000.0f;
        last_frame           = current_frame;

        while ( SDL_PollEvent( &e ) != 0 )
        {
            app_state.imgui_callback( &e );
            switch ( e.type )
            {
            case SDL_QUIT: app_state.is_running = false; break;
            case SDL_WINDOWEVENT:
                if ( e.window.event == SDL_WINDOWEVENT_RESIZED )
                {
                    app_state.window.data[ WindowParams::eWidth ] =
                        e.window.data1;
                    app_state.window.data[ WindowParams::eHeight ] =
                        e.window.data2;

                    app_state.on_resize( e.window.data1, e.window.data2 );
                }
                break;
            case SDL_KEYDOWN:
                app_state.input_system.keys[ e.key.keysym.scancode ] = true;
                break;
            case SDL_KEYUP:
                app_state.input_system.keys[ e.key.keysym.scancode ] = false;
                break;
            case SDL_MOUSEBUTTONDOWN:
                app_state.input_system.buttons[ e.button.button ] = true;
                break;
            case SDL_MOUSEBUTTONUP:
                app_state.input_system.buttons[ e.button.button ] = false;
                break;
            case SDL_MOUSEWHEEL:
            {
                if ( e.wheel.y > 0 )
                {
                    app_state.input_system.mouse_scroll = 1;
                }
                else if ( e.wheel.y < 0 )
                {
                    app_state.input_system.mouse_scroll = -1;
                }
                break;
            }
            default:
            {
                break;
            }
            }
        }
        i32 x, y;
        SDL_GetRelativeMouseState( &x, &y );
        app_state.input_system.mouse_offset[ 0 ] = x;
        app_state.input_system.mouse_offset[ 1 ] = y;
        SDL_GetGlobalMouseState( &x, &y );
        app_state.input_system.mouse_position[ 0 ] = x;
        app_state.input_system.mouse_position[ 1 ] = y;

        app_state.on_update( app_state.delta_time );
    }

    app_state.on_shutdown();
}

void app_shutdown()
{
    FT_ASSERT( app_state.is_inited );
    fluent::destroy_window( app_state.window );
}

const Window* get_app_window() { return &app_state.window; }

const InputSystem* get_app_input_system() { return &app_state.input_system; }

void app_set_ui_context( const UiContext& )
{
    // Bad but ok for now
    app_state.imgui_callback = ImGui_ImplSDL2_ProcessEvent;
}

u32 get_time() { return SDL_GetTicks(); }

f32 get_delta_time() { return app_state.delta_time; }

} // namespace fluent
