#pragma once

#include <string>
#include "fluent/fluent.hpp"

namespace fluent
{
struct Texture
{
    std::string filename;
    ImageDesc   desc;
    u64         size = 0;
    void*       data = nullptr;

    Texture() : filename("reserved slot"), data(nullptr)
    {
    }
    Texture(const std::string& directory, const std::string& filename);
    ~Texture();
};

struct Material
{
    u32 base_color_texture        = 0;
    u32 normal_texture            = 0;
    u32 ambient_occlusion_texture = 0;
    u32 emissive_texture          = 0;
    // pbr mr
    u32 metallic_roughness_texture = 0;
};

struct Mesh
{
    Material material;
    u32      first_vertex = 0;
    u32      vertex_count = 0;
    u32      first_index  = 0;
    u32      index_count  = 0;
};

enum class VertexComponent : u8
{
    ePosition = BIT(0),
    eNormal   = BIT(1),
    eTexcoord = BIT(2),
    eTangent  = BIT(3)
};
MAKE_ENUM_FLAG(u8, VertexComponent);

using VertexComponents = VertexComponent;

struct Model
{
    // TODO: Remove
    std::string directory;

    VertexLayout vertex_layout;
    u32          vertex_stride = 0;

    std::vector<Mesh>    meshes;
    std::vector<f32>     vertices;
    std::vector<u32>     indices;
    std::vector<Texture> textures;
};

Model create_triangle();

void fill_vertex_layout(VertexLayout& layout, VertexComponents components);
void load_model(Model& model, VertexComponents vertex_components, const std::string& filename);

} // namespace fluent