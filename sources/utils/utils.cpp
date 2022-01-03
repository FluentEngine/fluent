#include <fstream>
#include "utils/utils.hpp"

namespace fluent
{
std::vector<u32> read_file_binary(const char* filename)
{
    std::ifstream file(filename, std::ios_base::binary);
    FT_ASSERT(file.is_open() && "Failed to open file");

    std::vector<char> res = { std::istreambuf_iterator(file), std::istreambuf_iterator<char>() };
    return std::vector<uint32_t>(
        reinterpret_cast<uint32_t*>(res.data()), reinterpret_cast<uint32_t*>(res.data() + res.size()));
}
} // namespace fluent