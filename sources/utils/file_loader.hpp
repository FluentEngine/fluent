#pragma once

#include <vector>
#include "core/base.hpp"
#include "renderer/renderer_backend.hpp"

namespace fluent
{
std::vector<u32> read_file_binary(const std::string& filename);
} // namespace fluent