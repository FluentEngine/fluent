#pragma once

#include "base/base.h"
#include "window/window.h"

typedef bool ( *ft_application_init_callback )( uint32_t, char**, void* );
typedef void ( *ft_application_update_callback )( float, void* );
typedef void ( *ft_application_shutdown_callback )( void* );
typedef void ( *ft_application_resize_callback )( uint32_t, uint32_t, void* );

struct ft_application_info
{
	uint32_t                         argc;
	char**                           argv;
	struct ft_window_info            window_info;
	enum ft_log_level                log_level;
	ft_application_init_callback     on_init;
	ft_application_update_callback   on_update;
	ft_application_shutdown_callback on_shutdown;
	ft_application_resize_callback   on_resize;
	void*                            user_data;
};

FT_API bool
ft_app_init( const struct ft_application_info* info );

FT_API void
ft_app_run( void );

FT_API void
ft_app_shutdown( void );

FT_API void
ft_app_request_exit( void );

FT_API const struct ft_window*
ft_get_app_window( void );

FT_API struct ft_wsi_info*
ft_get_wsi_info( void );
