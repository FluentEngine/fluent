#include "core/application.hpp"
#include "resource_manager/resource_manager.hpp"
#include "resource_manager/resources.hpp"
#include "utils/model_loader.hpp"

namespace fluent
{
void ModelLoader::count_stride(const GeometryLoadDesc* desc)
{
    m_stride += 3;
    if (desc->load_normals)
    {
        m_normal_offset = static_cast<i32>(m_stride);
        m_stride += NORMAL_COMPONENT_COUNT;
    }

    if (desc->load_tex_coords)
    {
        m_texcoord_offset = static_cast<i32>(m_stride);
        m_stride += TEXCOORD_COMPONENT_COUNT;
    }

    if (desc->load_tangents)
    {
        m_tangents_offset = static_cast<i32>(m_stride);
        m_stride += TANGENTS_COMPONENT_COUNT;
    }

    if (desc->load_bitangents)
    {
        m_bitangents_offset = static_cast<i32>(m_stride);
        m_stride += BITANGENTS_COMPONENT_COUNT;
    }
}

GeometryDesc ModelLoader::load(const GeometryLoadDesc* desc)
{
    m_flip_uvs  = desc->flip_uvs;
    m_directory = std::string(desc->filename.substr(0, desc->filename.find_last_of('/')));
    count_stride(desc);
    return load(desc->filename);
}

GeometryDesc ModelLoader::load(const std::string& filename)
{
    Assimp::Importer importer;
    const aiScene*   scene = importer.ReadFile(
          get_app_models_directory() + filename,
          aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

    FT_ASSERT(scene || !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || scene->mRootNode);

    GeometryDesc data;
    process_node(data, scene->mRootNode, scene);
    fill_vertex_layout(&data.vertex_layout);
    return data;
}

void ModelLoader::process_node(GeometryDesc& model, aiNode* node, const aiScene* scene)
{
    for (uint32_t i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[ node->mMeshes[ i ] ];
        model.nodes.push_back(process_mesh(mesh, scene));
    }

    for (uint32_t i = 0; i < node->mNumChildren; i++)
    {
        process_node(model, node->mChildren[ i ], scene);
    }
}

GeometryDesc::GeometryDataNode ModelLoader::process_mesh(aiMesh* mesh, const aiScene* scene)
{
    // data to fill
    std::vector<f32>        vertices(mesh->mNumVertices * m_stride);
    std::vector<u32>        indices;
    std::vector<Ref<Image>> textures;

    // walk through each of the mesh's vertices
    for (u32 i = 0; i < mesh->mNumVertices; i++)
    {
        u32 idx             = i * m_stride;
        vertices[ idx ]     = mesh->mVertices[ i ].x;
        vertices[ idx + 1 ] = mesh->mVertices[ i ].y;
        vertices[ idx + 2 ] = mesh->mVertices[ i ].z;
        // normals
        if (mesh->HasNormals() && m_normal_offset > -1)
        {
            idx                 = i * m_stride + m_normal_offset;
            vertices[ idx ]     = mesh->mNormals[ i ].x;
            vertices[ idx + 1 ] = mesh->mNormals[ i ].y;
            vertices[ idx + 2 ] = mesh->mNormals[ i ].z;
        }
        // texture coordinates
        if (mesh->mTextureCoords[ 0 ] && m_texcoord_offset > -1)
        {
            idx                 = i * m_stride + m_texcoord_offset;
            vertices[ idx ]     = mesh->mTextureCoords[ 0 ][ i ].x;
            vertices[ idx + 1 ] = mesh->mTextureCoords[ 0 ][ i ].y;
        }
        else if (m_texcoord_offset > -1)
        {
            idx                 = i * m_stride + m_texcoord_offset;
            vertices[ idx ]     = 0;
            vertices[ idx + 1 ] = 0;
        }

        if (mesh->HasTangentsAndBitangents())
        {
            if (m_tangents_offset > -1)
            {
                idx = i * m_stride + m_tangents_offset;
                // tangent
                vertices[ idx ]     = mesh->mTangents[ i ].x;
                vertices[ idx + 1 ] = mesh->mTangents[ i ].y;
                vertices[ idx + 2 ] = mesh->mTangents[ i ].z;
            }

            if (m_bitangents_offset > -1)
            {
                // bitangent
                idx                 = i * m_stride + m_bitangents_offset;
                vertices[ idx ]     = mesh->mBitangents[ i ].x;
                vertices[ idx + 1 ] = mesh->mBitangents[ i ].y;
                vertices[ idx + 2 ] = mesh->mBitangents[ i ].z;
            }
        }
    }

    for (u32 i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[ i ];

        for (u32 j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[ j ]);
    }

    aiMaterial* material = scene->mMaterials[ mesh->mMaterialIndex ];

    std::vector<Ref<Image>> diffuseMaps = load_material_textures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

    std::vector<Ref<Image>> normalMaps = load_material_textures(material, aiTextureType_NORMALS, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

    std::vector<Ref<Image>> specularMaps = load_material_textures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    std::vector<Ref<Image>> heightMaps = load_material_textures(material, aiTextureType_HEIGHT, "texture_height");
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    std::vector<Ref<Image>> ambientMaps = load_material_textures(material, aiTextureType_AMBIENT, "texture_ambient");
    textures.insert(textures.end(), ambientMaps.begin(), ambientMaps.end());

    // TODO:
    std::vector<Ref<Image>> aoMaps = load_material_textures(material, aiTextureType_LIGHTMAP, "texture_ao");
    textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());

    std::vector<Ref<Image>> emissiveMaps = load_material_textures(material, aiTextureType_EMISSIVE, "texture_emissive");
    textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());

    std::vector<Ref<Image>> metalness_maps =
        load_material_textures(material, aiTextureType_METALNESS, "texture_metalness");
    textures.insert(textures.end(), metalness_maps.begin(), metalness_maps.end());
    // meshMaterial.metalness = prevTexSize != textures.size() ? textures.size() - 1 : -1;

    std::vector<Ref<Image>> metal_roughness_maps =
        load_material_textures(material, aiTextureType_UNKNOWN, "texture_metal_roughness");
    textures.insert(textures.end(), metal_roughness_maps.begin(), metal_roughness_maps.end());

    return GeometryDesc::GeometryDataNode{ vertices, indices };
}

std::vector<Ref<Image>> ModelLoader::load_material_textures(aiMaterial* mat, aiTextureType type, std::string type_name)
{
    return {};
}

void ModelLoader::fill_vertex_layout(VertexLayout* layout)
{
    layout->binding_desc_count            = 1;
    layout->binding_descs[ 0 ].binding    = 0;
    layout->binding_descs[ 0 ].stride     = m_stride * sizeof(f32);
    layout->binding_descs[ 0 ].input_rate = VertexInputRate::eVertex;

    u32 idx = 0;
    // Positions always exist
    {
        layout->attribute_descs[ idx ].location = 0;
        layout->attribute_descs[ idx ].binding  = 0;
        layout->attribute_descs[ idx ].format   = Format::eR32G32B32Sfloat;
        layout->attribute_descs[ idx ].offset   = 0;
        idx++;
    }
    // Normals
    if (m_normal_offset > -1)
    {
        layout->attribute_descs[ idx ].location = idx;
        layout->attribute_descs[ idx ].binding  = 0;
        layout->attribute_descs[ idx ].offset   = m_normal_offset * sizeof(f32);
        layout->attribute_descs[ idx ].format   = Format::eR32G32B32Sfloat;
        idx++;
    }
    // TexCoords
    if (m_texcoord_offset > -1)
    {
        layout->attribute_descs[ idx ].location = idx;
        layout->attribute_descs[ idx ].binding  = 0;
        layout->attribute_descs[ idx ].offset   = m_texcoord_offset * sizeof(f32);
        layout->attribute_descs[ idx ].format   = Format::eR32G32Sfloat;
        idx++;
    }
    // Tangents
    if (m_tangents_offset > -1)
    {
        layout->attribute_descs[ idx ].location = idx;
        layout->attribute_descs[ idx ].binding  = 0;
        layout->attribute_descs[ idx ].offset   = m_tangents_offset * sizeof(f32);
        layout->attribute_descs[ idx ].format   = Format::eR32G32B32Sfloat;
        idx++;
    }
    // Bitangents
    if (m_bitangents_offset > -1)
    {
        layout->attribute_descs[ idx ].location = idx;
        layout->attribute_descs[ idx ].binding  = 0;
        layout->attribute_descs[ idx ].offset   = m_bitangents_offset * sizeof(f32);
        layout->attribute_descs[ idx ].format   = Format::eR32G32B32Sfloat;
        idx++;
    }

    layout->attribute_desc_count = idx;
}

} // namespace fluent
