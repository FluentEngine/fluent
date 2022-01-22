#pragma once

#include <unordered_map>
#include <list>
#include "fluent/fluent.hpp"
#include "map_generator.hpp"
#include "mesh.hpp"

class TerrainChunk
{
private:
    fluent::Device const* m_device;
    fluent::Vector2       m_position;
    f32                   m_size;
    Mesh                  m_mesh;
    fluent::Matrix4       m_transform;
    b32                   m_is_visible;

public:
    void create(fluent::Device const* device, fluent::Vector2 coord, u32 size);
    void update(fluent::Vector2 const& viewer_position, f32 max_view_dist);
    void destroy();

    void set_visible(b32 visible)
    {
        m_is_visible = visible;
    }

    b32 is_visible() const
    {
        return m_is_visible;
    }

    const fluent::Matrix4& get_transform() const
    {
        return m_transform;
    }

    const Mesh& get_mesh() const
    {
        return m_mesh;
    }
};

class EndlessTerrain
{
private:
    static constexpr f32 MAX_VIEW_DIST = 300.0f;

    const fluent::Device* m_device;

    fluent::Vector2 m_viewer_position;

    u32          m_chunk_size;
    i32          m_chunks_visible;
    TerrainTypes m_terrain_types;

    std::unordered_map<fluent::Vector2, TerrainChunk> m_terrain_chunk_map;
    std::list<TerrainChunk*>                          m_visible_last_update;

    void update_visible_chunks();

public:
    void create(const fluent::Device& device);
    void destroy();

    void update(const fluent::Vector3& viewer_position);

    std::unordered_map<fluent::Vector2, TerrainChunk>& get_chunks()
    {
        return m_terrain_chunk_map;
    }
};
