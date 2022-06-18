#include <SDL.h>
#ifdef VULKAN_BACKEND
#include <SDL_vulkan.h>
#endif
#include "wsi/wsi.h"
#include "window.h"
#include "application.h"
#include "input.h"

#define MAX_INSTANCE_EXTENSION_COUNT 3

struct ApplicationState
{
	b32              is_inited;
	b32              is_running;
	struct Window    window;
	InitCallback     on_init;
	UpdateCallback   on_update;
	ShutdownCallback on_shutdown;
	ResizeCallback   on_resize;
	f32              delta_time;
	struct WsiInfo   wsi_info;
	const char*      extensions[ MAX_INSTANCE_EXTENSION_COUNT ];
};

static struct ApplicationState app_state;

#ifdef VULKAN_BACKEND
static void
ft_create_vulkan_surface( void* window, void* instance, void** p )
{
	VkSurfaceKHR surface;
	SDL_Vulkan_CreateSurface(
	    ( SDL_Window* ) ( ( struct Window* ) window )->handle,
	    ( VkInstance ) instance,
	    &surface );
	*p = surface;
}
#endif

static void
ft_window_get_size( void* window, u32* width, u32* height )
{
	window_get_size( ( struct Window* ) window, width, height );
}

static void
ft_window_get_framebuffer_size( void* window, u32* width, u32* height )
{
	window_get_framebuffer_size( ( struct Window* ) window, width, height );
}

void
app_init( const struct ApplicationConfig* config )
{
	FT_ASSERT( config->argv );
	FT_ASSERT( config->on_init );
	FT_ASSERT( config->on_update );
	FT_ASSERT( config->on_shutdown );
	FT_ASSERT( config->on_resize );

	app_state.window = create_window( &config->window_info );

	app_state.on_init     = config->on_init;
	app_state.on_update   = config->on_update;
	app_state.on_shutdown = config->on_shutdown;
	app_state.on_resize   = config->on_resize;

	struct WsiInfo* wsi_info = &app_state.wsi_info;
	wsi_info->window         = &app_state.window;
#ifdef VULKAN_BACKEND
	SDL_Vulkan_GetInstanceExtensions(
	    ( SDL_Window* ) app_state.window.handle,
	    &wsi_info->vulkan_instance_extension_count,
	    NULL );
	SDL_Vulkan_GetInstanceExtensions(
	    ( SDL_Window* ) app_state.window.handle,
	    &wsi_info->vulkan_instance_extension_count,
	    app_state.extensions );
	wsi_info->vulkan_instance_extensions = app_state.extensions;
	wsi_info->create_vulkan_surface      = ft_create_vulkan_surface;
#endif
	wsi_info->get_window_size      = ft_window_get_size;
	wsi_info->get_framebuffer_size = ft_window_get_framebuffer_size;

	log_init( FT_INFO );

	app_state.is_inited = 1;
}

void
app_run()
{
	FT_ASSERT( app_state.is_inited );

	app_state.on_init();

	app_state.is_running = 1;

	SDL_Event e;

	u32 last_frame       = 0.0f;
	app_state.delta_time = 0.0;

	static u32 width, height;
	window_get_size( &app_state.window, &width, &height );

	while ( app_state.is_running )
	{
		u32 current_frame    = get_time();
		app_state.delta_time = ( f32 ) ( current_frame - last_frame ) / 1000.0f;
		last_frame           = current_frame;

		while ( SDL_PollEvent( &e ) != 0 )
		{
			switch ( e.type )
			{
			case SDL_QUIT:
			{
				app_state.is_running = 0;
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
	log_shutdown();
	destroy_window( &app_state.window );
}

void
app_request_exit()
{
	app_state.is_running = 0;
}

const struct Window*
get_app_window()
{
	return &app_state.window;
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

struct WsiInfo*
get_ft_wsi_info()
{
	return &app_state.wsi_info;
}
