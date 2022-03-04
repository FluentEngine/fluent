#include <fstream>
#include <tinyimageformat_apis.h>
#include <tinyddsloader.h>
#include <stb_image.h>
#include "utils/image_loader.hpp"

namespace fluent
{
ImageDesc read_dds_image( const std::string& filename,
                          b32 flip,
                          u64* size,
                          void** data )
{
    using namespace tinyddsloader;

    ImageDesc image_desc {};

    DDSFile dds;
    auto ret = dds.Load( filename.c_str() );
    FT_ASSERT( ret == Result::Success );

    if ( flip )
    {
        dds.Flip();
    }

    // TODO: mip levels
    auto image_data = dds.GetImageData();

    image_desc.width  = image_data->m_width;
    image_desc.height = image_data->m_height;
    image_desc.depth  = image_data->m_depth;

    // TODO: formats
    image_desc.format = ( Format ) TinyImageFormat_FromDXGI_FORMAT(
        ( TinyImageFormat_DXGI_FORMAT ) dds.GetFormat() );
    image_desc.mip_levels  = dds.GetMipCount();
    image_desc.layer_count = dds.GetArraySize();
    *size                  = image_data->m_memSlicePitch;

    u8* loaded_data = new u8[ image_data->m_memSlicePitch ];
    std::memcpy( *data, image_data->m_mem, image_data->m_memSlicePitch );
    *data = loaded_data;
    return image_desc;
}

ImageDesc read_image_stb( const std::string& filename,
                          b32 flip,
                          u64* size,
                          void** data )
{
    ImageDesc image_desc {};

    stbi_set_flip_vertically_on_load( flip );
    int tex_width, tex_height, tex_channels;
    *data = stbi_load( filename.c_str(),
                       &tex_width,
                       &tex_height,
                       &tex_channels,
                       STBI_rgb_alpha );

    stbi_set_flip_vertically_on_load( !flip );

    image_desc.width       = tex_width;
    image_desc.height      = tex_height;
    image_desc.depth       = 1;
    image_desc.format      = Format::eR8G8B8A8Unorm;
    image_desc.mip_levels  = 1;
    image_desc.layer_count = 1;
    *size                  = tex_width * tex_height * 4;

    return image_desc;
}

ImageDesc read_image_data( const std::string& filename,
                           b32 flip,
                           u64* size,
                           void** data )
{
    std::string file_ext = filename.substr( filename.find_last_of( '.' ) );

    ImageDesc image_desc {};

    if ( file_ext == ".dds" )
    {
        image_desc = read_dds_image( filename, flip, size, data );
    }
    else
    {
        image_desc = read_image_stb( filename, flip, size, data );
    }

    return image_desc;
}

void release_image_data( void* data ) { delete[]( u8* ) data; }
} // namespace fluent
