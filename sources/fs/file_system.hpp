#pragma once

#include <string>
#include "core/base.hpp"
#include "renderer/renderer_backend.hpp"

namespace fluent::fs
{
void init( char** argv );
void set_shaders_directory( const std::string& path );
void set_textures_directory( const std::string& path );
void set_models_directory( const std::string& path );

const std::string& get_shaders_directory();
const std::string& get_textures_directory();
const std::string& get_models_directory();

std::vector<char> read_file_binary( const std::string& filename );

ImageDesc read_image_data( const std::string& filename,
                           b32                flip,
                           u64*               size,
                           void**             data );

void release_image_data( void* data );

} // namespace fluent::fs
