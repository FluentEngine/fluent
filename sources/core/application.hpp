#pragma once

#include "core/base.hpp"

namespace fluent
{

struct Window;

using InitCallback = void (*)();
using UpdateCallback = void (*)(f64 deltaTime);
using RenderCallback = void (*)();
using LoadCallback = void (*)(u32 width, u32 height);
using UnloadCallback = void (*)();
using ShutdownCallback = void (*)();

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
    RenderCallback on_render;
    LoadCallback on_load;
    UnloadCallback on_unload;
    ShutdownCallback on_shutdown;
};

void application_init(const ApplicationConfig* state);
void application_run();
void application_shutdown();

const char* get_app_name();
const Window* get_app_window();

} // namespace fluent