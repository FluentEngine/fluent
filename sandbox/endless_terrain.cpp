#include <cmath>
#include "mesh_generator.hpp"
#include "endless_terrain.hpp"

using namespace fluent;

void TerrainChunk::create(fluent::Device const* device, fluent::Vector2 coord, u32 size)
{
    m_device = device;
    m_size   = ( f32 ) size;

    m_position = coord * m_size;
    Vector3 position(m_position.x, 0, m_position.y);

    auto noise_map = MapGenerator::generate_noise_map(m_position);
    auto mesh_data = MeshGenerator::generate_mesh(noise_map, 40.0f, 1);

    MeshDesc mesh_desc{};
    mesh_desc.vertices_size = mesh_data.vertices.size() * 3;
    mesh_desc.vertices      = ( f32* ) mesh_data.vertices.data();
    mesh_desc.uvs_size      = mesh_data.uvs.size() * 2;
    mesh_desc.uvs           = ( f32* ) mesh_data.uvs.data();
    mesh_desc.indices_size  = mesh_data.indices.size();
    mesh_desc.indices       = ( u32* ) mesh_data.indices.data();

    m_mesh.create(*m_device, mesh_desc);

    set_visible(false);

    m_transform = translate(Matrix4(1.0f), position);
}

void TerrainChunk::destroy()
{
    m_mesh.destroy(*m_device);
}

void TerrainChunk::update(const fluent::Vector2& viewer_position, f32 max_view_dist)
{
    // TODO
    b32 res = glm::length(m_position - viewer_position) <= max_view_dist;
    if (m_mesh.get_vertex_buffer().buffer)
    {
        set_visible(true);
    }
    set_visible(res);
}

void EndlessTerrain::create(const Device& device)
{
    m_chunk_size     = MapGenerator::get_map_size() - 1;
    m_device         = &device;
    m_chunks_visible = std::round(MAX_VIEW_DIST / m_chunk_size);

    m_terrain_types.push_back({ 0, Vector3(0.214, 0.751, 0.925) });      // water
    m_terrain_types.push_back({ 0.289f, Vector3(0.966, 0.965, 0.613) }); // sand
    m_terrain_types.push_back({ 0.323f, Vector3(0.331, 1.0, 0.342) });   // ground
    m_terrain_types.push_back({ 0.473f, Vector3(0.225, 0.225, 0.217) }); // mountains
    m_terrain_types.push_back({ 0.806f, Vector3(1.0, 1.0, 1.0) });       // snow

    std::sort(m_terrain_types.begin(), m_terrain_types.end(), [](const auto& a, const auto& b) -> bool {
        return a.height > b.height;
    });
}

void EndlessTerrain::destroy()
{
    for (auto& [ pos, chunk ] : m_terrain_chunk_map)
    {
        chunk.destroy();
    }
}

void EndlessTerrain::update_visible_chunks()
{
    for (auto* chunk : m_visible_last_update)
    {
        chunk->set_visible(false);
    }
    m_visible_last_update.clear();

    i32 current_chunk_coord_x = std::round(m_viewer_position.x / ( f32 ) m_chunk_size);
    i32 current_chunk_coord_y = std::round(m_viewer_position.y / ( f32 ) m_chunk_size);

    for (i32 y_offset = -m_chunks_visible; y_offset <= m_chunks_visible; y_offset++)
    {
        for (i32 x_offset = -m_chunks_visible; x_offset <= m_chunks_visible; x_offset++)
        {
            Vector2 viewer_chunk_coord(current_chunk_coord_x + x_offset, current_chunk_coord_y + y_offset);

            auto it = m_terrain_chunk_map.find(viewer_chunk_coord);
            if (it != m_terrain_chunk_map.end())
            {
                it->second.update(m_viewer_position, MAX_VIEW_DIST);
                if (it->second.is_visible())
                {
                    m_visible_last_update.emplace_back(&it->second);
                }
            }
            else
            {
                auto it = m_terrain_chunk_map.emplace(viewer_chunk_coord, TerrainChunk());
                (*(it.first)).second.create(m_device, viewer_chunk_coord, m_chunk_size);
            }
        }
    }
}

void EndlessTerrain::update(const fluent::Vector3& viewer_position)
{
    m_viewer_position = Vector2(viewer_position.x, viewer_position.z);
    update_visible_chunks();
}
