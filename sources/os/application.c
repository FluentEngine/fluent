#include <SDL.h>
#ifdef VULKAN_BACKEND
#include <SDL_vulkan.h>
#endif
#include "wsi/wsi.h"
#include "window.h"
#include "application.h"
#include "input.h"

#define MAX_INSTANCE_EXTENSION_COUNT 3

struct application_state
{
	bool                 is_inited;
	bool                 is_running;
	struct ft_window     window;
	ft_init_callback     on_init;
	ft_update_callback   on_update;
	ft_shutdown_callback on_shutdown;
	ft_resize_callback   on_resize;
	float                delta_time;
	struct ft_wsi_info   wsi_info;
	const char*          extensions[ MAX_INSTANCE_EXTENSION_COUNT ];
};

static struct application_state app_state;

#ifdef VULKAN_BACKEND
static void
create_vulkan_surface( void* window, void* instance, void** p )
{
	VkSurfaceKHR surface;
	SDL_Vulkan_CreateSurface(
	    ( SDL_Window* ) ( ( struct ft_window* ) window )->handle,
	    ( VkInstance ) instance,
	    &surface );
	*p = surface;
}
#endif

static void
window_get_size( void* window, uint32_t* width, uint32_t* height )
{
	ft_window_get_size( ( struct ft_window* ) window, width, height );
}

static void
window_get_framebuffer_size( void* window, uint32_t* width, uint32_t* height )
{
	ft_window_get_framebuffer_size( ( struct ft_window* ) window,
	                                width,
	                                height );
}

void
ft_app_init( const struct ft_application_config* config )
{
	FT_ASSERT( config->argv );
	FT_ASSERT( config->on_init );
	FT_ASSERT( config->on_update );
	FT_ASSERT( config->on_shutdown );
	FT_ASSERT( config->on_resize );

	app_state.window = ft_create_window( &config->window_info );

	app_state.on_init     = config->on_init;
	app_state.on_update   = config->on_update;
	app_state.on_shutdown = config->on_shutdown;
	app_state.on_resize   = config->on_resize;

	struct ft_wsi_info* wsi_info = &app_state.wsi_info;
	wsi_info->window             = &app_state.window;
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
	wsi_info->create_vulkan_surface      = create_vulkan_surface;
#endif
	wsi_info->get_window_size      = window_get_size;
	wsi_info->get_framebuffer_size = window_get_framebuffer_size;

	ft_log_init( FT_INFO );

	app_state.is_inited = 1;
}

void
ft_app_run()
{
	FT_ASSERT( app_state.is_inited );

	app_state.on_init();

	app_state.is_running = 1;

	SDL_Event e;

	uint32_t last_frame  = 0.0f;
	app_state.delta_time = 0.0;

	static uint32_t width, height;
	ft_window_get_size( &app_state.window, &width, &height );

	while ( app_state.is_running )
	{
		uint32_t current_frame = ft_get_time();
		app_state.delta_time =
		    ( float ) ( current_frame - last_frame ) / 1000.0f;
		last_frame = current_frame;

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
					uint32_t w, h;
					ft_window_get_size( &app_state.window, &w, &h );
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
ft_app_shutdown()
{
	FT_ASSERT( app_state.is_inited );
	ft_log_shutdown();
	ft_destroy_window( &app_state.window );
}

void
ft_app_request_exit()
{
	app_state.is_running = 0;
}

const struct ft_window*
ft_get_app_window()
{
	return &app_state.window;
}

uint32_t
ft_get_time()
{
	return SDL_GetTicks();
}

float
ft_get_delta_time()
{
	return app_state.delta_time;
}

struct ft_wsi_info*
ft_get_wsi_info()
{
	return &app_state.wsi_info;
}
