#pragma once

#include <string>
#include <unordered_map>
#include "core/base.hpp"
#include "renderer/renderer_backend.hpp"
#include "resource_manager/resources.hpp"

namespace fluent
{
struct BufferLoadDesc
{
    BufferDesc  buffer_desc;
    u32         offset = 0;
    u32         size   = 0;
    const void* data   = nullptr;
};

struct ImageLoadDesc
{
    ImageDesc   image_desc;
    std::string filename;
    u32         size = 0;
    const void* data = nullptr;
    b32         flip = false;
};

struct GeometryLoadDesc
{
    std::string         filename;
    const GeometryDesc* desc;
    b32                 load_normals;
    b32                 load_tex_coords;
    b32                 load_tangents;
    b32                 load_bitangents;
    b32                 flip_uvs;
};

class ResourceManager
{
private:
    static const Device*  m_device;
    static CommandPool*   m_command_pool;
    static CommandBuffer* m_cmd;
    static Queue*         m_queue;

    static std::unordered_map<u32, Ref<Image>>    m_images;
    static std::unordered_map<u32, Ref<Geometry>> m_geometries;

    static void create_staging_buffer(u32 size, BaseBuffer** buffer);

public:
    static void init(const Device* device);
    static void shutdown();

    static void load_buffer(Ref<Buffer>& buffer, const BufferLoadDesc* desc);
    static void release_buffer(Ref<Buffer>& buffer);
    static void load_image(Ref<Image>& image, const ImageLoadDesc* desc);
    static void load_color_image(Ref<Image>& image, const VectorInt4& color);
    static void release_image(Ref<Image>& image);
    static void load_geometry(Ref<Geometry>& geometry, const GeometryLoadDesc* desc);
    static void release_geometry(Ref<Geometry>& geometry);
};
} // namespace fluent
