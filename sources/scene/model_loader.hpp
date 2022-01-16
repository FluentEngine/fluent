#pragma once

#include <vector>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "core/base.hpp"
#include "math/math.hpp"
#include "renderer/renderer.hpp"

namespace fluent
{

struct Material
{
    i32 diffuse           = -1;
    i32 specular          = -1;
    i32 normal            = -1;
    i32 height            = -1;
    i32 ambient           = -1;
    i32 ambient_occlusion = -1;
    i32 emissive          = -1;
    i32 metal_roughness   = -1;
};

struct Model
{
    struct Mesh
    {
        std::vector<f32> vertices;
        std::vector<u32> indices;
        Buffer           vertex_buffer;
        Buffer           index_buffer;
        Matrix4          transform;
        Material         material;

        void InitMesh(const Device& device);

        Mesh(const Device& device, std::vector<f32> vertices, std::vector<u32> indices, Material material);
    };

    std::vector<Mesh>  meshes;
    std::vector<Image> textures;
};

struct LoadModelDescription
{
    std::string filename;
    b32         load_normals;
    b32         load_tex_coords;
    b32         load_tangents;
    b32         load_bitangents;
    b32         flip_uvs;
};

class ModelLoader
{
private:
    static constexpr f32 NORMAL_COMPONENT_COUNT     = 3;
    static constexpr f32 TEXCOORD_COMPONENT_COUNT   = 2;
    static constexpr f32 TANGENTS_COMPONENT_COUNT   = 3;
    static constexpr f32 BITANGENTS_COMPONENT_COUNT = 3;

    i32 normal_offset     = -1;
    i32 texcoord_offset   = -1;
    i32 tangents_offset   = -1;
    i32 bitangents_offset = -1;

    b32 flip_uvs;

    struct LoadedTexture
    {
        std::string name;
        Image       texture;
    };

    u32                        stride = 0;
    std::vector<LoadedTexture> textures_loaded;
    std::string                directory;

    const Device* device;

    [[nodiscard]] Model load(const std::string& filename);

    void process_node(Model& model, aiNode* node, const aiScene* scene);

    Model::Mesh process_mesh(aiMesh* mesh, const aiScene* scene);

    std::vector<LoadedTexture> load_material_textures(aiMaterial* mat, aiTextureType type, std::string type_name);

    void count_stride(const LoadModelDescription& desc);

public:
    [[nodiscard]] Model load(const Device* device, const LoadModelDescription& desc);

    void get_vertex_binding_description(u32& count, VertexBindingDesc* desc);
    void get_vertex_attribute_description(u32& count, VertexAttributeDesc* desc);
};

void destroy_model(const Device& device, Model& model);

} // namespace fluent
