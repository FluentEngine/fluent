#pragma once

#include <vector>
#include "core/base.hpp"
#include "renderer/renderer.hpp"
namespace fluent
{

std::vector<u32> read_file_binary(const std::string& filename);
ImageDesc        read_dds_image(const char* filename, b32 flip, void** data);
void             release_dds_image(void* data);
ImageDesc        read_image(const char* filename, b32 flip, void** data);
void             release_image(void* data);

} // namespace fluent