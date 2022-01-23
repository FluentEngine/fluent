#include "core/application.hpp"
#include "scene/model_loader.hpp"

namespace fluent
{

void Model::Mesh::InitMesh(const Device& device)
{
    BufferDesc bufferDesc{};
    bufferDesc.descriptor_type = DescriptorType::eVertexBuffer;
    bufferDesc.size            = vertices.size() * sizeof(vertices[ 0 ]);

    vertex_buffer = create_buffer(device, bufferDesc);

    BufferUpdateDesc buffer_update_desc{};
    buffer_update_desc.buffer = &vertex_buffer;
    buffer_update_desc.data   = vertices.data();
    buffer_update_desc.offset = 0;
    buffer_update_desc.size   = vertices.size() * sizeof(vertices[ 0 ]);
    update_buffer(device, buffer_update_desc);

    bufferDesc = {};

    bufferDesc.descriptor_type = DescriptorType::eIndexBuffer;
    bufferDesc.size            = indices.size() * sizeof(indices[ 0 ]);

    index_buffer = create_buffer(device, bufferDesc);

    buffer_update_desc        = {};
    buffer_update_desc.buffer = &index_buffer;
    buffer_update_desc.data   = indices.data();
    buffer_update_desc.offset = 0;
    buffer_update_desc.size   = indices.size() * sizeof(indices[ 0 ]);
    update_buffer(device, buffer_update_desc);
}

Model::Mesh::Mesh(const Device& device, std::vector<float> vertices, std::vector<uint32_t> indices, Material material)
    : vertices(std::move(vertices)), indices(std::move(indices)), material(std::move(material))
{
    InitMesh(device);
}

void ModelLoader::count_stride(const LoadModelDescription& desc)
{
    stride += 3;
    if (desc.load_normals)
    {
        normal_offset = static_cast<i32>(stride);
        stride += NORMAL_COMPONENT_COUNT;
    }

    if (desc.load_tex_coords)
    {
        texcoord_offset = static_cast<i32>(stride);
        stride += TEXCOORD_COMPONENT_COUNT;
    }

    if (desc.load_tangents)
    {
        tangents_offset = static_cast<i32>(stride);
        stride += TANGENTS_COMPONENT_COUNT;
    }

    if (desc.load_bitangents)
    {
        bitangents_offset = static_cast<i32>(stride);
        stride += BITANGENTS_COMPONENT_COUNT;
    }
}

Model ModelLoader::load(const Device* device, const LoadModelDescription& desc)
{
    this->device = device;
    flip_uvs     = desc.flip_uvs;
    directory    = std::string(desc.filename.substr(0, desc.filename.find_last_of('/')));
    count_stride(desc);
    return load(desc.filename);
}

Model ModelLoader::load(const std::string& filename)
{
    Assimp::Importer importer;
    const aiScene*   scene = importer.ReadFile(
          get_app_models_directory() + filename,
          aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

    FT_ASSERT(scene || !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || scene->mRootNode);

    Model model;
    process_node(model, scene->mRootNode, scene);

    model.textures.resize(textures_loaded.size());
    for (uint32_t i = 0; i < textures_loaded.size(); ++i)
        model.textures[ i ] = textures_loaded[ i ].texture;

    return model;
}

void ModelLoader::process_node(Model& model, aiNode* node, const aiScene* scene)
{
    for (uint32_t i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[ node->mMeshes[ i ] ];
        model.meshes.push_back(process_mesh(mesh, scene));
        auto& t = node->mTransformation;
        model.meshes.back().transform =
            Matrix4(t.a1, t.b1, t.c1, t.d1, t.a2, t.b2, t.c2, t.d2, t.a3, t.b3, t.c3, t.d3, t.a4, t.b4, t.c4, t.d4);
    }

    for (uint32_t i = 0; i < node->mNumChildren; i++)
    {
        process_node(model, node->mChildren[ i ], scene);
    }
}

Model::Mesh ModelLoader::process_mesh(aiMesh* mesh, const aiScene* scene)
{
    // data to fill
    std::vector<f32>           vertices(mesh->mNumVertices * stride);
    std::vector<u32>           indices;
    std::vector<LoadedTexture> textures;

    // walk through each of the mesh's vertices
    for (u32 i = 0; i < mesh->mNumVertices; i++)
    {
        u32 idx             = i * stride;
        vertices[ idx ]     = mesh->mVertices[ i ].x;
        vertices[ idx + 1 ] = mesh->mVertices[ i ].y;
        vertices[ idx + 2 ] = mesh->mVertices[ i ].z;
        // normals
        if (mesh->HasNormals() && normal_offset > -1)
        {
            idx                 = i * stride + normal_offset;
            vertices[ idx ]     = mesh->mNormals[ i ].x;
            vertices[ idx + 1 ] = mesh->mNormals[ i ].y;
            vertices[ idx + 2 ] = mesh->mNormals[ i ].z;
        }
        // texture coordinates
        if (mesh->mTextureCoords[ 0 ] && texcoord_offset > -1)
        {
            idx                 = i * stride + texcoord_offset;
            vertices[ idx ]     = mesh->mTextureCoords[ 0 ][ i ].x;
            vertices[ idx + 1 ] = mesh->mTextureCoords[ 0 ][ i ].y;
        }
        else if (texcoord_offset > -1)
        {
            idx                 = i * stride + texcoord_offset;
            vertices[ idx ]     = 0;
            vertices[ idx + 1 ] = 0;
        }

        if (mesh->HasTangentsAndBitangents())
        {
            if (tangents_offset > -1)
            {
                idx = i * stride + tangents_offset;
                // tangent
                vertices[ idx ]     = mesh->mTangents[ i ].x;
                vertices[ idx + 1 ] = mesh->mTangents[ i ].y;
                vertices[ idx + 2 ] = mesh->mTangents[ i ].z;
            }

            if (bitangents_offset > -1)
            {
                // bitangent
                idx                 = i * stride + bitangents_offset;
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

    Material meshMaterial{};

    int                        prevTexSize = textures.size();
    std::vector<LoadedTexture> diffuseMaps = load_material_textures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    meshMaterial.diffuse = prevTexSize != textures.size() ? textures.size() - 1 : -1;
    prevTexSize          = textures.size();

    std::vector<LoadedTexture> normalMaps = load_material_textures(material, aiTextureType_NORMALS, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    meshMaterial.normal = prevTexSize != textures.size() ? textures.size() - 1 : -1;
    prevTexSize         = textures.size();

    std::vector<LoadedTexture> specularMaps =
        load_material_textures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    meshMaterial.specular = prevTexSize != textures.size() ? textures.size() - 1 : -1;
    prevTexSize           = textures.size();

    std::vector<LoadedTexture> heightMaps = load_material_textures(material, aiTextureType_HEIGHT, "texture_height");
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
    meshMaterial.height = prevTexSize != textures.size() ? textures.size() - 1 : -1;
    prevTexSize         = textures.size();

    std::vector<LoadedTexture> ambientMaps = load_material_textures(material, aiTextureType_AMBIENT, "texture_ambient");
    textures.insert(textures.end(), ambientMaps.begin(), ambientMaps.end());
    meshMaterial.ambient = prevTexSize != textures.size() ? textures.size() - 1 : -1;
    prevTexSize          = textures.size();

    // TODO:
    std::vector<LoadedTexture> aoMaps = load_material_textures(material, aiTextureType_LIGHTMAP, "texture_ao");
    textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());
    meshMaterial.ambient_occlusion = prevTexSize != textures.size() ? textures.size() - 1 : -1;
    prevTexSize                    = textures.size();

    std::vector<LoadedTexture> emissiveMaps =
        load_material_textures(material, aiTextureType_EMISSIVE, "texture_emissive");
    textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());
    meshMaterial.emissive = prevTexSize != textures.size() ? textures.size() - 1 : -1;
    prevTexSize           = textures.size();

    std::vector<LoadedTexture> metalness_maps =
        load_material_textures(material, aiTextureType_METALNESS, "texture_metalness");
    textures.insert(textures.end(), metalness_maps.begin(), metalness_maps.end());
    // meshMaterial.metalness = prevTexSize != textures.size() ? textures.size() - 1 : -1;
    prevTexSize = textures.size();

    std::vector<LoadedTexture> metal_roughness_maps =
        load_material_textures(material, aiTextureType_UNKNOWN, "texture_metal_roughness");
    textures.insert(textures.end(), metal_roughness_maps.begin(), metal_roughness_maps.end());
    meshMaterial.metal_roughness = prevTexSize != textures.size() ? textures.size() - 1 : -1;
    prevTexSize                  = textures.size();

    return Model::Mesh(*device, vertices, indices, meshMaterial);
}

std::vector<ModelLoader::LoadedTexture> ModelLoader::load_material_textures(
    aiMaterial* mat, aiTextureType type, std::string type_name)
{
    std::vector<LoadedTexture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        b32 skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++)
        {
            if (textures_loaded[ j ].name == type_name)
            {
                textures.push_back(textures_loaded[ j ]);
                skip = true;
                break;
            }
        }

        if (!skip)
        {
            std::string texName = str.C_Str();

            LoadedTexture texture{};
            texture.name    = type_name;
            texture.texture = load_image_from_file(
                *device, (directory + "/" + texName).c_str(), ResourceState::eShaderReadOnly, flip_uvs);
            textures.push_back(texture);
            textures_loaded.push_back(texture);
        }
    }
    return textures;
}

void ModelLoader::get_vertex_binding_description(u32& count, VertexBindingDesc* desc)
{
    desc[ 0 ].binding    = 0;
    desc[ 0 ].stride     = stride * sizeof(f32);
    desc[ 0 ].input_rate = VertexInputRate::eVertex;

    count = 1;
}

void ModelLoader::get_vertex_attribute_description(u32& count, VertexAttributeDesc* desc)
{
    u32 idx = 0;
    // Positions always exist
    {
        desc[ idx ].location = 0;
        desc[ idx ].binding  = 0;
        desc[ idx ].format   = Format::eR32G32B32Sfloat;
        desc[ idx ].offset   = 0;
        idx++;
    }
    // Normals
    if (normal_offset > -1)
    {
        desc[ idx ].location = idx;
        desc[ idx ].binding  = 0;
        desc[ idx ].offset   = normal_offset * sizeof(f32);
        desc[ idx ].format   = Format::eR32G32B32Sfloat;
        idx++;
    }
    // TexCoords
    if (texcoord_offset > -1)
    {
        desc[ idx ].location = idx;
        desc[ idx ].binding  = 0;
        desc[ idx ].offset   = texcoord_offset * sizeof(f32);
        desc[ idx ].format   = Format::eR32G32Sfloat;
        idx++;
    }
    // Tangents
    if (tangents_offset > -1)
    {
        desc[ idx ].location = idx;
        desc[ idx ].binding  = 0;
        desc[ idx ].offset   = tangents_offset * sizeof(f32);
        desc[ idx ].format   = Format::eR32G32B32Sfloat;
        idx++;
    }
    // Bitangents
    if (bitangents_offset > -1)
    {
        desc[ idx ].location = idx;
        desc[ idx ].binding  = 0;
        desc[ idx ].offset   = bitangents_offset * sizeof(f32);
        desc[ idx ].format   = Format::eR32G32B32Sfloat;
    }

    count = idx + 1;
}

void destroy_model(const Device& device, Model& model)
{
    for (auto& mesh : model.meshes)
    {
        destroy_buffer(device, mesh.vertex_buffer);
        destroy_buffer(device, mesh.index_buffer);
    }

    for (auto& image : model.textures)
    {
        destroy_image(device, image);
    }
}

} // namespace fluent
