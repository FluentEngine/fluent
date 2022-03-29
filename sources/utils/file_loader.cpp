#include <fstream>

#include "utils/file_loader.hpp"

namespace fluent
{
std::vector<char> read_file_binary( const std::string& filename )
{
    std::ifstream file( filename, std::ios_base::binary );
    FT_ASSERT( file.is_open() && "Failed to open file" );

    return { std::istreambuf_iterator( file ),
             std::istreambuf_iterator<char>() };
}

std::vector<char> read_shader( const std::string& shader_name )
{
#ifdef VULKAN_BACKEND
    return read_file_binary( shader_name + ".glsl.spv" );
#endif
#ifdef D3D12_BACKEND
    return read_file_binary( shader_name + ".hlsl.dxil" );
#endif
    return {};
}
} // namespace fluent
