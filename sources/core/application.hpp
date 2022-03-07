#pragma once

#include "core/base.hpp"
#include "core/window.hpp"

namespace fluent
{
struct InputSystem;
struct UiContext;

using InitCallback     = void ( * )();
using UpdateCallback   = void ( * )( f32 deltaTime );
using ShutdownCallback = void ( * )();
using ResizeCallback   = void ( * )( u32 width, u32 height );

struct ApplicationConfig
{
    u32              argc;
    char**           argv;
    WindowDesc       window_desc;
    LogLevel         log_level;
    InitCallback     on_init;
    UpdateCallback   on_update;
    ShutdownCallback on_shutdown;
    ResizeCallback   on_resize;
};

void app_init( const ApplicationConfig* state );
void app_run();
void app_shutdown();

void app_set_ui_context( const UiContext& context );

const Window*      get_app_window();
const InputSystem* get_app_input_system();

u32 get_time();
f32 get_delta_time();

} // namespace fluent
