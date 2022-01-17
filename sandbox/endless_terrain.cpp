#include <cmath>
#include "endless_terrain.hpp"

using namespace fluent;

void EndlessTerrain::TerrainChunk::create(fluent::Device const* device, fluent::Vector2 coord, u32 size)
{
    m_device = device;
    m_size   = ( f32 ) size;

    m_position = coord * m_size;
    Vector3 position(m_position.x, 0, m_position.y);
    // // TEST STUFF
    // std::vector<Vector3> vertices = {
    //     Vector3(-0.5, 3.0, 0.5),
    //     Vector3(0.5, 3.0, 0.5),
    //     Vector3(0.5, 3.0, -0.5),
    //     Vector3(-0.5, 3.0, -0.5),
    // };

    // std::vector<Vector2> uvs(4);

    // std::vector<u32> indices = { 0, 1, 2, 2, 3, 0 };

    // MeshDesc mesh_desc{};
    // mesh_desc.vertices_size = vertices.size() * 3;
    // mesh_desc.vertices      = ( f32* ) vertices.data();
    // mesh_desc.uvs_size      = uvs.size() * 2;
    // mesh_desc.uvs           = ( f32* ) uvs.data();
    // mesh_desc.indices_size  = indices.size();
    // mesh_desc.indices       = indices.data();

    // m_mesh.create(*m_device, mesh_desc);

    m_transform = scale(translate(Matrix4(1.0f), position), Vector3(10.0f));

    m_is_visible = false;
}

void EndlessTerrain::TerrainChunk::destroy()
{
    // m_mesh.destroy(*m_device);
}

void EndlessTerrain::TerrainChunk::update(const fluent::Vector2& viewer_position)
{
    // TODO:
    f32 dst_to_nearest_edge = std::numeric_limits<f32>::max();

    f32 length = glm::length(viewer_position - Vector2(m_position.x - m_size, m_position.y - m_size));
    if (length < dst_to_nearest_edge)
    {
        length = dst_to_nearest_edge;
    }
    length = glm::length(viewer_position - Vector2(m_position.x - m_size, m_position.y + m_size));
    if (length < dst_to_nearest_edge)
    {
        length = dst_to_nearest_edge;
    }
    length = glm::length(viewer_position - Vector2(m_position.x + m_size, m_position.y + m_size));
    if (length < dst_to_nearest_edge)
    {
        length = dst_to_nearest_edge;
    }
    length = glm::length(viewer_position - Vector2(m_position.x + m_size, m_position.y - m_size));
    if (length < dst_to_nearest_edge)
    {
        length = dst_to_nearest_edge;
    }

    m_is_visible = dst_to_nearest_edge <= MAX_VIEW_DIST;
}

void EndlessTerrain::create(const Device& device)
{
    m_chunk_size     = MapGenerator::get_map_size() - 1;
    m_device         = &device;
    m_chunks_visible = std::round(MAX_VIEW_DIST / m_chunk_size);
}

void EndlessTerrain::destroy()
{
    for (auto& [ pos, chunk ] : terrain_chunk_map)
    {
        chunk.destroy();
    }
}

void EndlessTerrain::update_visible_chunks()
{
    i32 current_chunk_coord_x = std::round(m_viewer_position.x / ( f32 ) m_chunk_size);
    i32 current_chunk_coord_y = std::round(m_viewer_position.y / ( f32 ) m_chunk_size);

    for (i32 y_offset = -m_chunks_visible; y_offset <= m_chunks_visible; y_offset++)
    {
        for (i32 x_offset = -m_chunks_visible; x_offset <= m_chunks_visible; x_offset++)
        {
            Vector2 viewer_chunk_coord(current_chunk_coord_x + x_offset, current_chunk_coord_y + y_offset);

            auto it = terrain_chunk_map.find(viewer_chunk_coord);
            if (it != terrain_chunk_map.end())
            {
                it->second.update(m_viewer_position);
            }
            else
            {
                auto it = terrain_chunk_map.emplace(viewer_chunk_coord, TerrainChunk());
                (*(it.first)).second.create(m_device, viewer_chunk_coord * 10.0f, m_chunk_size);
                (*(it.first)).second.set_test_mesh(test_mesh);
            }
        }
    }
}

void EndlessTerrain::update(const fluent::Vector3& viewer_position)
{
    m_viewer_position = Vector2(viewer_position.x, viewer_position.z) / 10.0f;
    update_visible_chunks();
}
