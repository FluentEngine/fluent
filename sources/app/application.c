#include <time.h>
#include "wsi/wsi.h"
#include "time/timer.h"
#include "application.h"
#include "window/input.h"

struct application_state
{
	bool                             is_inited;
	bool                             is_running;
	struct ft_window*                window;
	ft_application_init_callback     on_init;
	ft_application_update_callback   on_update;
	ft_application_shutdown_callback on_shutdown;
	ft_application_resize_callback   on_resize;
	struct ft_wsi_info               wsi_info;
	uint32_t                         width;
	uint32_t                         height;
	void*                            user_data;
	bool                             resized;
};

static struct application_state app_state;

static void
app_resize_callback( struct ft_window* window, uint32_t w, uint32_t h, void* p )
{
	app_state.resized = true;
}

void
ft_app_init( const struct ft_application_info* config )
{
	FT_ASSERT( config->argv );
	FT_ASSERT( config->on_init );
	FT_ASSERT( config->on_update );
	FT_ASSERT( config->on_shutdown );
	FT_ASSERT( config->on_resize );

	ft_log_init( config->log_level );

	memset( &app_state, 0, sizeof( app_state ) );

	app_state.window = ft_create_window( &config->window_info );

	app_state.on_init     = config->on_init;
	app_state.on_update   = config->on_update;
	app_state.on_shutdown = config->on_shutdown;
	app_state.on_resize   = config->on_resize;
	app_state.user_data   = config->user_data;

	struct ft_wsi_info* wsi_info = &app_state.wsi_info;
	wsi_info->window             = app_state.window;
	wsi_info->get_vulkan_instance_extensions =
	    ft_window_get_vulkan_instance_extensions;
	wsi_info->create_vulkan_surface = ft_window_create_vulkan_surface;
	wsi_info->get_window_size       = ft_window_get_size;
	wsi_info->get_framebuffer_size  = ft_window_get_framebuffer_size;

	ft_ticks_init();
	app_state.is_inited = 1;

	ft_window_set_resize_callback( app_state.window, app_resize_callback );
}

void
ft_app_run()
{
	FT_ASSERT( app_state.is_inited );

	app_state.is_running = 1;

	uint32_t last_frame = 0.0f;

	ft_window_get_size( app_state.window, &app_state.width, &app_state.height );

	app_state.on_init( app_state.user_data );

	while ( app_state.is_running )
	{
		uint32_t current_frame = ft_get_ticks();
		float delta_time = ( float ) ( current_frame - last_frame ) / 1000.0f;
		last_frame       = current_frame;

		ft_input_update();

		ft_poll_events();

		if ( app_state.resized )
		{
			app_state.resized = false;
			ft_window_get_size( app_state.window,
			                    &app_state.width,
			                    &app_state.height );
			app_state.on_resize( app_state.width,
			                     app_state.height,
			                     app_state.user_data );
		}

		app_state.on_update( delta_time, app_state.user_data );

		app_state.is_running = !ft_window_should_close( app_state.window );
	}

	app_state.on_shutdown( app_state.user_data );
}

void
ft_app_shutdown()
{
	FT_ASSERT( app_state.is_inited );
	ft_ticks_shutdown();
	ft_log_shutdown();
	ft_destroy_window( app_state.window );
}

void
ft_app_request_exit()
{
	app_state.is_running = 0;
}

const struct ft_window*
ft_get_app_window()
{
	return app_state.window;
}

struct ft_wsi_info*
ft_get_wsi_info()
{
	return &app_state.wsi_info;
}
