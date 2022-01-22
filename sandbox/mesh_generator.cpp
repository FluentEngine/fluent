#include "mesh_generator.hpp"

MeshData MeshGenerator::generate_mesh(const Map& map, f32 height_multiplier, u32 lod)
{
    using namespace fluent;

    static constexpr u32 MAP_SIZE = MapGenerator::get_map_size();

    f32 top_left_x = (( f32 ) MAP_SIZE - 1.0f) / -2.0f;
    f32 top_left_z = (( f32 ) MAP_SIZE - 1.0f) / 2.0f;

    u32 mesh_simplification_increment = lod;
    u32 vertices_per_line             = (MAP_SIZE - 1) / mesh_simplification_increment + 1;

    MeshData mesh_data;
    mesh_data.create_mesh(vertices_per_line, vertices_per_line);

    u32 vertex_index = 0;

    for (u32 y = 0; y < MAP_SIZE; y += mesh_simplification_increment)
    {
        for (u32 x = 0; x < MAP_SIZE; x += mesh_simplification_increment)
        {
            f32 height = map.data[ x + y * MAP_SIZE ];

            mesh_data.vertices[ vertex_index ] =
                Vector3(( f32 ) (top_left_x + x), height * height_multiplier, ( f32 ) (top_left_z - y));
            mesh_data.uvs[ vertex_index ] = Vector2(( f32 ) x / ( f32 ) MAP_SIZE, ( f32 ) y / ( f32 ) MAP_SIZE);

            if ((x < (MAP_SIZE - 1)) && (y < (MAP_SIZE - 1)))
            {
                mesh_data.add_triangle(
                    vertex_index, vertex_index + vertices_per_line + 1, vertex_index + vertices_per_line);
                mesh_data.add_triangle(vertex_index, vertex_index + 1, vertex_index + vertices_per_line + 1);
            }

            vertex_index++;
        }
    }

    return mesh_data;
}