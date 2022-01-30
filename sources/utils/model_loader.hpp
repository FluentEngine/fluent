#pragma once

#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "resource_manager/resource_manager.hpp"

namespace fluent
{
class ModelLoader
{
private:
    static constexpr f32 NORMAL_COMPONENT_COUNT     = 3;
    static constexpr f32 TEXCOORD_COMPONENT_COUNT   = 2;
    static constexpr f32 TANGENTS_COMPONENT_COUNT   = 3;
    static constexpr f32 BITANGENTS_COMPONENT_COUNT = 3;

    i32 m_normal_offset     = -1;
    i32 m_texcoord_offset   = -1;
    i32 m_tangents_offset   = -1;
    i32 m_bitangents_offset = -1;

    b32 m_flip_uvs;

    u32         m_stride = 0;
    std::string m_directory;

    [[nodiscard]] GeometryDesc load(const std::string& filename);

    void process_node(GeometryDesc& model, aiNode* node, const aiScene* scene);

    GeometryDesc::GeometryDataNode process_mesh(aiMesh* mesh, const aiScene* scene);

    std::vector<Ref<Image>> load_material_textures(aiMaterial* mat, aiTextureType type, std::string type_name);

    void count_stride(const GeometryLoadDesc* desc);
    void fill_vertex_layout(VertexLayout* layout);

public:
    [[nodiscard]] GeometryDesc load(const GeometryLoadDesc* desc);
};
} // namespace fluent
