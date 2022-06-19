#pragma once

#include "base/base.h"
#include "log/log.h"
#include "window.h"

typedef void ( *ft_init_callback )( void );
typedef void ( *ft_update_callback )( float deltaTime );
typedef void ( *ft_shutdown_callback )( void );
typedef void ( *ft_resize_callback )( uint32_t width, uint32_t height );

struct ft_application_config
{
	uint32_t              argc;
	char**                argv;
	struct ft_window_info window_info;
	enum ft_log_level     log_level;
	ft_init_callback      on_init;
	ft_update_callback    on_update;
	ft_shutdown_callback  on_shutdown;
	ft_resize_callback    on_resize;
};

FT_API void
ft_app_init( const struct ft_application_config* state );

FT_API void
ft_app_run( void );

FT_API void
ft_app_shutdown( void );

FT_API void
ft_app_request_exit( void );

FT_API const struct ft_window*
ft_get_app_window( void );

FT_API uint32_t
ft_get_time( void );
FT_API float
ft_get_delta_time( void );

FT_API struct ft_wsi_info*
ft_get_wsi_info( void );
