#pragma once

#include "base/base.h"
#include "log/log.h"
#include "window.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef void ( *InitCallback )( void );
	typedef void ( *UpdateCallback )( f32 deltaTime );
	typedef void ( *ShutdownCallback )( void );
	typedef void ( *ResizeCallback )( u32 width, u32 height );

	struct ApplicationConfig
	{
		u32               argc;
		char**            argv;
		struct WindowInfo window_info;
		enum LogLevel     log_level;
		InitCallback      on_init;
		UpdateCallback    on_update;
		ShutdownCallback  on_shutdown;
		ResizeCallback    on_resize;
	};

	void
	app_init( const struct ApplicationConfig* state );

	void
	app_run( void );

	void
	app_shutdown( void );

	void
	app_request_exit( void );

	const struct Window*
	get_app_window( void );

	u32
	get_time( void );
	f32
	get_delta_time( void );

	struct WsiInfo*
	get_ft_wsi_info( void );

#ifdef __cplusplus
}
#endif
