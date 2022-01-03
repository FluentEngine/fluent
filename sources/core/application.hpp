#pragma once

#include "core/base.hpp"

namespace fluent
{

struct Window;

using InitCallback = void (*)();
using UpdateCallback = void (*)(f64 deltaTime);
using ShutdownCallback = void (*)();
using ResizeCallback = void (*)(u32 width, u32 height);

struct ApplicationConfig
{
    const char* title = nullptr;
    u32 x;
    u32 y;
    u32 width;
    u32 height;
    LogLevel log_level;
    InitCallback on_init;
    UpdateCallback on_update;
    ShutdownCallback on_shutdown;
    ResizeCallback on_resize;
};

void application_init(const ApplicationConfig* state);
void application_run();
void application_shutdown();

const char* get_app_name();
const Window* get_app_window();

} // namespace fluent