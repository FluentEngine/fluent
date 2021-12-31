#pragma once

#include "spdlog/spdlog.h"

#ifdef FLUENT_DEBUG
#define FT_TRACE(...) spdlog::trace(__VA_ARGS__)
#define FT_INFO(...) spdlog::info(__VA_ARGS__)
#define FT_WARN(...) spdlog::warn(__VA_ARGS__)
#define FT_ERROR(...) spdlog::error(__VA_ARGS__)
#else
#define FT_TRACE(...)
#define FT_INFO(...)
#define FT_WARN(...)
#define FT_ERROR(...)
#endif