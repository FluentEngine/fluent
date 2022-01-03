#pragma once

#include <vector>
#include "core/base.hpp"

namespace fluent
{
std::vector<u32> read_file_binary(const std::string& filename);
} // namespace fluent