#pragma once

#include <vector>
#include "fluent/fluent.hpp"
#include "map_generator.hpp"

struct MeshData
{
    std::vector<fluent::Vector3> vertices;
    std::vector<fluent::Vector2> uvs;
    std::vector<u32>             indices;
    u32                          triangle_index = 0;

    void create_mesh(u32 width, u32 height)
    {
        vertices.resize(width * height);
        uvs.resize(width * height);
        indices.resize((width - 1) * (height - 1) * 6);
    }

    void add_triangle(u32 a, u32 b, u32 c)
    {
        indices[ triangle_index ]     = a;
        indices[ triangle_index + 1 ] = b;
        indices[ triangle_index + 2 ] = c;
        triangle_index += 3;
    }
};

class MeshGenerator
{
private:
    f32 m_min_height = 0.0f;

public:
    void set_min_height(f32 min_value);

    [[nodiscard]] MeshData generate_mesh(const Map& map, f32 height_multiplier, u32 lod);
};