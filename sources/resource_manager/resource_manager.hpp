#pragma once

#include <unordered_map>
#include "core/base.hpp"
#include "renderer/renderer.hpp"
#include "resource_manager/resources.hpp"

namespace fluent
{
struct BufferLoadDesc
{
    Buffer**    p_buffer = nullptr;
    BufferDesc  buffer_desc;
    u32         offset = 0;
    u32         size   = 0;
    const void* data   = nullptr;
};

struct GeometryLoadDesc
{
    std::string         filename;
    const GeometryData* data;
    Geometry**          p_geometry;
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

    static std::unordered_map<u32, Geometry*> m_geometries;

    static void create_staging_buffer(u32 size, Buffer** buffer);

public:
    static void init(const Device* device);
    static void shutdown();

    static void load_buffer(const BufferLoadDesc* desc);
    static void load_geometry(const GeometryLoadDesc* desc);
    static void free_geometry(Geometry* geometry);
};
} // namespace fluent
