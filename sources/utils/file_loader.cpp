#include <fstream>

#include "utils/file_loader.hpp"

namespace fluent
{
std::vector<u32> read_file_binary( const std::string& filename )
{
    std::ifstream file( filename, std::ios_base::binary );
    FT_ASSERT( file.is_open() && "Failed to open file" );

    std::vector<char> res = { std::istreambuf_iterator( file ),
                              std::istreambuf_iterator<char>() };
    return std::vector<u32>(
        reinterpret_cast<u32*>( res.data() ),
        reinterpret_cast<u32*>( res.data() + res.size() ) );
}
} // namespace fluent