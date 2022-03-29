#pragma once

#include <vector>
#include "core/base.hpp"

namespace fluent
{
std::vector<char> read_file_binary( const std::string& filename );
std::vector<char> read_shader( const std::string& shader_name );
} // namespace fluent
