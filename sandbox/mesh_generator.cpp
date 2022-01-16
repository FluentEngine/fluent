#include "mesh_generator.hpp"

MeshData MeshGenerator::generate_mesh(const Map& map, f32 height_multiplier)
{
    using namespace fluent;

    u32 width  = map.width;
    u32 height = map.height;

    f32 top_left_x = (( f32 ) width - 1.0f) / -2.0f;
    f32 top_left_z = (( f32 ) height - 1.0f) / 2.0f;

    MeshData mesh_data;
    mesh_data.create_mesh(width, height);

    u32 vertex_index = 0;

    for (u32 y = 0; y < height; ++y)
    {
        for (u32 x = 0; x < width; ++x)
        {
            f32 map_height = map.data[ x * map.bpp + y * map.bpp * width ];

            mesh_data.vertices[ vertex_index ] =
                Vector3(( f32 ) (top_left_x + x), map_height * height_multiplier, ( f32 ) (top_left_z - y));
            mesh_data.uvs[ vertex_index ] = Vector2(( f32 ) x / ( f32 ) width, ( f32 ) y / ( f32 ) height);

            if ((x < (width - 1)) && (y < (height - 1)))
            {
                mesh_data.add_triangle(vertex_index, vertex_index + width + 1, vertex_index + width);
                mesh_data.add_triangle(vertex_index, vertex_index + 1, vertex_index + width + 1);
            }

            vertex_index++;
        }
    }

    return mesh_data;
}