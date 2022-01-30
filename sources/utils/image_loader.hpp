#pragma once

#include <vector>
#include "core/base.hpp"
#include "renderer/renderer_backend.hpp"

namespace fluent
{
ImageDesc read_dds_image(const std::string& filename, b32 flip, u32* size, void** data);
void      release_dds_image_data(void* data);
ImageDesc read_image(const std::string& filename, b32 flip, u32* size, void** data);
void      release_image_data(void* data);

} // namespace fluent