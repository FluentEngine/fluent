#include <chrono>
#include <filesystem>
#include <SDL.h>
#include <imgui_impl_sdl.h>
#include "core/window.hpp"
#include "core/application.hpp"
#include "core/input.hpp"
#include "fs/fs.hpp"

namespace fluent
{
// defined in input.cpp
InputSystem*
get_input_system();

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
	ImGuiCallback    imgui_callback;
};

static ApplicationState app_state;

static inline logger_LogLevel
to_logger_log_level( LogLevel level )
{
	switch ( level )
	{
	case LogLevel::eTrace: return LogLevel_TRACE;
	case LogLevel::eDebug: return LogLevel_DEBUG;
	case LogLevel::eInfo: return LogLevel_INFO;
	case LogLevel::eWarn: return LogLevel_WARN;
	case LogLevel::eError: return LogLevel_ERROR;
	default: return logger_LogLevel( 0 );
	}
}

void
app_init( const ApplicationConfig* config )
{
	FT_ASSERT( config->argv );
	FT_ASSERT( config->on_init );
	FT_ASSERT( config->on_update );
	FT_ASSERT( config->on_shutdown );
	FT_ASSERT( config->on_resize );

	app_state.window = fluent::create_window( config->window_info );

	app_state.on_init        = config->on_init;
	app_state.on_update      = config->on_update;
	app_state.on_shutdown    = config->on_shutdown;
	app_state.on_resize      = config->on_resize;
	app_state.imgui_callback = []( const SDL_Event* ) -> bool { return false; };

	int result = logger_initConsoleLogger( nullptr );
	FT_ASSERT( result );
	logger_autoFlush( 1000 );
	logger_setLevel( to_logger_log_level( config->log_level ) );
	init_input_system( &app_state.window );

	fs::init( config->argv );

	app_state.is_inited = true;
}

void
app_run()
{
	FT_ASSERT( app_state.is_inited );

	app_state.on_init();

	app_state.is_running = true;

	SDL_Event e;

	u32 last_frame       = 0.0f;
	app_state.delta_time = 0.0;

	InputSystem* input_system = get_input_system();

	static u32 width, height;
	window_get_size( &app_state.window, &width, &height );

	while ( app_state.is_running )
	{
		update_input_system();

		u32 current_frame    = get_time();
		app_state.delta_time = ( current_frame - last_frame ) / 1000.0f;
		last_frame           = current_frame;

		while ( SDL_PollEvent( &e ) != 0 )
		{
			app_state.imgui_callback( &e );
			switch ( e.type )
			{
			case SDL_QUIT:
			{
				app_state.is_running = false;
				break;
			}
			case SDL_WINDOWEVENT:
			{
				if ( e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED )
				{
					u32 w, h;
					window_get_size( &app_state.window, &w, &h );
					if ( width == w && height == h )
					{
						break;
					}
					app_state.on_resize( w, h );
				}
				break;
			}
			case SDL_KEYDOWN:
			{
				input_system->keys[ e.key.keysym.scancode ] = true;
				break;
			}
			case SDL_KEYUP:
			{
				input_system->keys[ e.key.keysym.scancode ] = false;
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			{
				input_system->buttons[ e.button.button ] = true;
				break;
			}
			case SDL_MOUSEBUTTONUP:
			{
				input_system->buttons[ e.button.button ] = false;
				break;
			}
			case SDL_MOUSEWHEEL:
			{
				if ( e.wheel.y > 0 )
				{
					input_system->mouse_scroll = 1;
				}
				else if ( e.wheel.y < 0 )
				{
					input_system->mouse_scroll = -1;
				}
				break;
			}
			default:
			{
				break;
			}
			}
		}
		app_state.on_update( app_state.delta_time );
	}

	app_state.on_shutdown();
}

void
app_shutdown()
{
	FT_ASSERT( app_state.is_inited );
	logger_flush();
	fluent::destroy_window( app_state.window );
}

const Window*
get_app_window()
{
	return &app_state.window;
}

void
app_set_ui_context( const UiContext& )
{
	// Bad but ok for now
	app_state.imgui_callback = ImGui_ImplSDL2_ProcessEvent;
}

u32
get_time()
{
	return SDL_GetTicks();
}

f32
get_delta_time()
{
	return app_state.delta_time;
}

} // namespace fluent
