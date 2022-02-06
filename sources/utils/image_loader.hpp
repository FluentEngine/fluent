#pragma once

#include <vector>
#include "core/base.hpp"
#include "renderer/renderer_backend.hpp"

namespace fluent
{
ImageDesc read_image_data(const std::string& filename, b32 flip, u64* size, void** data);
void      release_image_data(void* data);
} // namespace fluent