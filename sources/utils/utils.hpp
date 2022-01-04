#pragma once

#include <vector>
#include "core/base.hpp"
#include "renderer/renderer.hpp"
namespace fluent
{

std::vector<u32> read_file_binary(const std::string& filename);
ImageDesc read_image_desc_from_dds(const char* filename);
void release_image_desc_dds(ImageDesc& desc);
ImageDesc read_image_desc(const char* filename);
void release_image_desc(ImageDesc& desc);

} // namespace fluent