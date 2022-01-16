#pragma once

#include <vector>
#include "fluent/fluent.hpp"

struct MeshDesc
{
    u32        vertices_size = 0;
    const f32* vertices      = nullptr;
    u32        uvs_size      = 0;
    const f32* uvs           = nullptr;
    u32        indices_size  = 0;
    const u32* indices       = nullptr;
};

class Mesh
{
private:
    fluent::Buffer m_vertex_buffer;
    fluent::Buffer m_index_buffer;

    u32 m_index_count = 0;

    std::vector<f32> convert_vertices(const MeshDesc& desc);

public:
    void create(const fluent::Device& device, const MeshDesc& desc);

    void destroy(const fluent::Device& device);

    const fluent::Buffer& get_vertex_buffer() const
    {
        return m_vertex_buffer;
    }

    const fluent::Buffer& get_index_buffer() const
    {
        return m_index_buffer;
    }

    u32 get_index_count() const
    {
        return m_index_count;
    }
};
