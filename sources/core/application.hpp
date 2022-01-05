#pragma once

#include "core/base.hpp"

namespace fluent
{

struct Window;
struct InputSystem;

using InitCallback = void (*)();
using UpdateCallback = void (*)(f64 deltaTime);
using ShutdownCallback = void (*)();
using ResizeCallback = void (*)(u32 width, u32 height);

struct ApplicationConfig
{
    u32 argc;
    char** argv;
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

void app_init(const ApplicationConfig* state);
void app_run();
void app_shutdown();

void app_set_shaders_directory(const std::string& path);
void app_set_textures_directory(const std::string& path);
void app_set_models_directory(const std::string& path);

const char* get_app_name();
const Window* get_app_window();
const InputSystem* get_app_input_system();

const std::string& get_app_shaders_directory();
const std::string& get_app_textures_directory();
const std::string& get_app_models_directory();

f32 get_time();
f32 get_delta_time();

} // namespace fluent