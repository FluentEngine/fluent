#pragma once

#include <unordered_map>
#include "fluent/fluent.hpp"
#include "map_generator.hpp"
#include "mesh.hpp"

class EndlessTerrain
{
    class TerrainChunk
    {
    private:
        fluent::Device const* m_device;
        fluent::Vector2       m_position;
        f32                   m_size;
        Mesh*                 m_mesh;
        fluent::Matrix4       m_transform;

        b32 m_is_visible;

    public:
        void create(fluent::Device const* device, fluent::Vector2 coord, u32 size);
        void set_test_mesh(Mesh* mesh)
        {
            m_mesh = mesh;
        }
        void update(fluent::Vector2 const& viewer_position);
        void destroy();

        const fluent::Matrix4& get_transform() const
        {
            return m_transform;
        }

        const Mesh& get_mesh() const
        {
            return *m_mesh;
        }
    };

private:
    static constexpr f32 MAX_VIEW_DIST = 300.0f;

    const fluent::Device* m_device;
    Mesh*                 test_mesh;

    fluent::Vector2 m_viewer_position;

    u32 m_chunk_size;
    i32 m_chunks_visible;

    std::unordered_map<fluent::Vector2, TerrainChunk> terrain_chunk_map;

    void update_visible_chunks();

public:
    void create(const fluent::Device& device);
    void set_test_mesh(Mesh* mesh)
    {
        test_mesh = mesh;
    }
    void destroy();

    void update(const fluent::Vector3& viewer_position);

    std::unordered_map<fluent::Vector2, TerrainChunk>& get_chunks()
    {
        return terrain_chunk_map;
    }
};
