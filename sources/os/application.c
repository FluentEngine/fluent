#include <time.h>
#include "wsi/wsi.h"
#include "os/time/timer.h"
#include "application.h"
#include "input.h"

struct application_state
{
	bool                                is_inited;
	bool                                is_running;
	struct ft_window*                   window;
	ft_init_callback                    on_init;
	ft_update_callback                  on_update;
	ft_shutdown_callback                on_shutdown;
	ft_resize_callback                  on_resize;
	float                               delta_time;
	struct ft_wsi_info                  wsi_info;
	struct ft_application_callback_data callback_data;
};

static struct application_state app_state;

void
ft_input_update( int32_t wheel );

void
ft_app_init( const struct ft_application_info* config )
{
	FT_ASSERT( config->argv );
	FT_ASSERT( config->on_init );
	FT_ASSERT( config->on_update );
	FT_ASSERT( config->on_shutdown );
	FT_ASSERT( config->on_resize );

	ft_log_init( config->log_level );

	app_state.window = ft_create_window( &config->window_info );

	app_state.on_init                 = config->on_init;
	app_state.on_update               = config->on_update;
	app_state.on_shutdown             = config->on_shutdown;
	app_state.on_resize               = config->on_resize;
	app_state.callback_data.user_data = config->user_data;

	struct ft_wsi_info* wsi_info = &app_state.wsi_info;
	wsi_info->window             = app_state.window;
	wsi_info->get_vulkan_instance_extensions =
	    ft_window_get_vulkan_instance_extensions;
	wsi_info->create_vulkan_surface = ft_window_create_vulkan_surface;
	wsi_info->get_window_size       = ft_window_get_size;
	wsi_info->get_framebuffer_size  = ft_window_get_framebuffer_size;

	ft_ticks_init();
	app_state.is_inited = 1;
}

void
ft_app_run()
{
	FT_ASSERT( app_state.is_inited );

	app_state.is_running = 1;

	uint32_t last_frame  = 0.0f;
	app_state.delta_time = 0.0;

	ft_window_get_size( app_state.window,
	                    &app_state.callback_data.width,
	                    &app_state.callback_data.height );

	app_state.callback_data.delta_time = 0.0f;

	app_state.on_init( &app_state.callback_data );

	while ( app_state.is_running )
	{
		uint32_t current_frame = ft_get_ticks();
		app_state.callback_data.delta_time =
		    ( float ) ( current_frame - last_frame ) / 1000.0f;
		last_frame = current_frame;

		ft_input_update( 0 );

		ft_poll_events();

		app_state.on_update( &app_state.callback_data );

		app_state.is_running = !ft_window_should_close( app_state.window );
	}

	app_state.on_shutdown( &app_state.callback_data );
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
