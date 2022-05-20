#pragma once

#include "base/base.h"
#include "log/log.h"
#include "window.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef void ( *InitCallback )();
	typedef void ( *UpdateCallback )( f32 deltaTime );
	typedef void ( *ShutdownCallback )();
	typedef void ( *ResizeCallback )( u32 width, u32 height );

	typedef struct ApplicationConfig
	{
		u32              argc;
		char**           argv;
		WindowInfo       window_info;
		LogLevel         log_level;
		InitCallback     on_init;
		UpdateCallback   on_update;
		ShutdownCallback on_shutdown;
		ResizeCallback   on_resize;
	} ApplicationConfig;

	void
	app_init( const ApplicationConfig* state );

	void
	app_run();

	void
	app_shutdown();

	void
	app_request_exit();

	const Window*
	get_app_window();

	u32
	get_time();
	f32
	get_delta_time();

	struct WsiInfo*
	get_ft_wsi_info();

#ifdef __cplusplus
}
#endif
