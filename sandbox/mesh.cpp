#include "mesh.hpp"

std::vector<f32> Mesh::convert_vertices(const MeshDesc& desc)
{
    std::vector<f32> converted_vertices(desc.vertices_size + desc.uvs_size);

    u32 vertex_index = 0;
    u32 uv_index     = 0;

    for (u32 i = 0; i < converted_vertices.size();)
    {
        converted_vertices[ i++ ] = desc.vertices[ vertex_index++ ];
        converted_vertices[ i++ ] = desc.vertices[ vertex_index++ ];
        converted_vertices[ i++ ] = desc.vertices[ vertex_index++ ];
        converted_vertices[ i++ ] = desc.uvs[ uv_index++ ];
        converted_vertices[ i++ ] = desc.uvs[ uv_index++ ];
    }

    return converted_vertices;
}

void Mesh::create(const fluent::Device& device, const MeshDesc& desc)
{
    using namespace fluent;

    m_index_count = desc.indices_size;

    auto converted_vertices = convert_vertices(desc);

    BufferDesc buffer_descs[ 2 ]      = {};
    buffer_descs[ 0 ].size            = converted_vertices.size() * sizeof(converted_vertices[ 0 ]);
    buffer_descs[ 0 ].resource_state  = ResourceState::eTransferDst;
    buffer_descs[ 0 ].descriptor_type = DescriptorType::eVertexBuffer;
    buffer_descs[ 0 ].data            = converted_vertices.data();
    buffer_descs[ 1 ].size            = desc.indices_size * sizeof(desc.indices[ 0 ]);
    buffer_descs[ 1 ].resource_state  = ResourceState::eTransferDst;
    buffer_descs[ 1 ].descriptor_type = DescriptorType::eIndexBuffer;
    buffer_descs[ 1 ].data            = desc.indices;

    m_vertex_buffer = create_buffer(device, buffer_descs[ 0 ]);
    m_index_buffer  = create_buffer(device, buffer_descs[ 1 ]);
}

void Mesh::destroy(const fluent::Device& device)
{
    destroy_buffer(device, m_vertex_buffer);
    destroy_buffer(device, m_index_buffer);
}