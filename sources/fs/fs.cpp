#include <filesystem>
#include <fstream>
#include <tinyimageformat_apis.h>
#include <tinyddsloader.h>
#include <stb_image.h>
#include "fs/fs.hpp"

namespace fluent::fs
{
struct
{
    std::string exec_path;
    std::string shaders_directory;
    std::string textures_directory;
    std::string models_directory;
} fs_data;

std::string
get_exec_path( char** argv )
{
    std::string exec_path =
        std::filesystem::weakly_canonical( std::filesystem::path( argv[ 0 ] ) )
            .parent_path()
            .string() +
        "/";

    return exec_path;
}

void
init( char** argv )
{
    fs_data.exec_path = get_exec_path( argv );
}

void
set_shaders_directory( const std::string& path )
{
    fs_data.shaders_directory =
        fs_data.exec_path + std::filesystem::path( path ).string() + "/";
}

void
set_textures_directory( const std::string& path )
{
    fs_data.textures_directory =
        fs_data.exec_path + std::filesystem::path( path ).string() + "/";
}

void
set_models_directory( const std::string& path )
{
    fs_data.models_directory =
        fs_data.exec_path + std::filesystem::path( path ).string() + "/";
}

const std::string&
get_shaders_directory()
{
    return fs_data.shaders_directory;
}

const std::string&
get_textures_directory()
{
    return fs_data.textures_directory;
}

const std::string&
get_models_directory()
{
    return fs_data.models_directory;
}

std::vector<char>
read_file_binary( const std::string& filename )
{
    std::ifstream file( filename, std::ios::ate | std::ios::binary );
    FT_ASSERT( file.is_open() && "Failed to open file" );

    size_t            file_size = static_cast<size_t>( file.tellg() );
    std::vector<char> buffer( file_size );

    file.seekg( 0 );
    file.read( buffer.data(), file_size );
    file.close();

    return buffer;
}

ImageDesc
read_dds_image( const std::string& filename, b32 flip, u64* size, void** data )
{
    using namespace tinyddsloader;

    ImageDesc image_desc {};

    DDSFile dds;
    auto    ret = dds.Load( filename.c_str() );
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
    std::memcpy( loaded_data, image_data->m_mem, image_data->m_memSlicePitch );
    *data = loaded_data;
    return image_desc;
}

ImageDesc
read_image_stb( const std::string& filename, b32 flip, u64* size, void** data )
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

ImageDesc
read_image_data( const std::string& filename, b32 flip, u64* size, void** data )
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

void
release_image_data( void* data )
{
    delete[]( u8* ) data;
}
} // namespace fluent::fs
