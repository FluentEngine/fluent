#include <fstream>
#include "tinyimageformat_apis.h"
#include "tinyddsloader.h"
#include "stb_image.h"
#include "utils/utils.hpp"

namespace fluent
{
std::vector<u32> read_file_binary(const std::string& filename)
{
    std::ifstream file(filename, std::ios_base::binary);
    FT_ASSERT(file.is_open() && "Failed to open file");

    std::vector<char> res = { std::istreambuf_iterator(file), std::istreambuf_iterator<char>() };
    return std::vector<u32>(reinterpret_cast<u32*>(res.data()), reinterpret_cast<u32*>(res.data() + res.size()));
}

ImageDesc read_image_desc_from_dds(const char* filename, b32 flip)
{
    using namespace tinyddsloader;

    ImageDesc image_desc{};

    DDSFile dds;
    auto    ret = dds.Load(filename);
    FT_ASSERT(ret == Result::Success);

    if (flip)
    {
        dds.Flip();
    }

    // TODO: mip levels
    auto image_data = dds.GetImageData();

    image_desc.width  = image_data->m_width;
    image_desc.height = image_data->m_height;
    image_desc.depth  = image_data->m_depth;

    // TODO: formats
    image_desc.format     = ( Format ) TinyImageFormat_FromDXGI_FORMAT(( TinyImageFormat_DXGI_FORMAT ) dds.GetFormat());
    image_desc.mip_levels = dds.GetMipCount();
    image_desc.layer_count = dds.GetArraySize();
    image_desc.data_size   = image_data->m_memSlicePitch;
    image_desc.data        = new u8[ image_data->m_memSlicePitch ];
    std::memcpy(image_desc.data, image_data->m_mem, image_data->m_memSlicePitch);

    return image_desc;
}

void release_image_desc_dds(ImageDesc& desc)
{
    delete[](u8*) desc.data;
}

ImageDesc read_image_desc(const char* filename, b32 flip)
{
    ImageDesc image_desc{};

    stbi_set_flip_vertically_on_load(flip);
    int tex_width, tex_height, tex_channels;
    image_desc.data      = stbi_load(filename, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
    image_desc.data_size = tex_width * tex_height * 4;
    stbi_set_flip_vertically_on_load(!flip);

    image_desc.width       = tex_width;
    image_desc.height      = tex_height;
    image_desc.depth       = 1;
    image_desc.format      = Format::eR8G8B8A8Unorm;
    image_desc.mip_levels  = 1;
    image_desc.layer_count = 1;

    return image_desc;
}

void release_image_desc(ImageDesc& desc)
{
    stbi_image_free(desc.data);
}

} // namespace fluent