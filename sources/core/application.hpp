#pragma once

#include "core/base.hpp"

namespace fluent
{

using UpdateCallback = void (*)(f64 deltaTime);
using RenderCallback = void (*)();
using LoadCallback = void (*)(u32 width, u32 height);
using UnloadCallback = void (*)();

struct ApplicationConfig
{
    const char* title = nullptr;
    u32 x;
    u32 y;
    u32 width;
    u32 height;
    UpdateCallback on_update;
    RenderCallback on_render;
    LoadCallback on_load;
    UnloadCallback on_unload;
};

void application_init(const ApplicationConfig* state);
void application_run();
void application_shutdown();

} // namespace fluent