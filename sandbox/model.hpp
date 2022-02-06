#pragma once

#include "fluent/fluent.hpp"

namespace fluent
{
struct Texture
{
};

struct Material
{
};

struct Mesh
{
    Material material;
    u32      vertex_offset = 0;
    u32      vertex_count  = 0;
    u32      vertex_stride = 0;
    u32      index_offset  = 0;
    u32      index_count   = 0;
};

struct Model
{
    VertexLayout vertex_layout;

    std::vector<Mesh>    meshes;
    std::vector<f32>     vertices;
    std::vector<u32>     indices;
    std::vector<Texture> textures;
};

Model create_triangle();

} // namespace fluent