#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "model.hpp"

namespace fluent
{
Texture::Texture(const std::string& directory, const std::string& filename) : data(nullptr), filename(filename)
{
    desc                 = read_image_data(directory + "/" + filename, true, &size, &data);
    desc.descriptor_type = DescriptorType::eSampledImage;
}

Texture::~Texture()
{
    if (data)
    {
        release_image_data(data);
    }
    data = nullptr;
}

Model create_triangle()
{
    Model model;

    VertexLayout& layout = model.vertex_layout;

    layout.binding_desc_count            = 1;
    layout.binding_descs[ 0 ].binding    = 0;
    layout.binding_descs[ 0 ].stride     = 3 * sizeof(f32);
    layout.binding_descs[ 0 ].input_rate = VertexInputRate::eVertex;
    layout.attribute_desc_count          = 1;
    layout.attribute_descs[ 0 ].binding  = 0;
    layout.attribute_descs[ 0 ].format   = Format::eR32G32B32Sfloat;
    layout.attribute_descs[ 0 ].location = 0;
    layout.attribute_descs[ 0 ].offset   = 0;

    model.vertex_stride = 3;
    model.vertices      = { -0.5, -0.5, 0.0, 0.5, -0.5, 0.0, 0.0, 0.5, 0.0 };
    model.indices       = { 0, 1, 2 };

    Mesh& mesh        = model.meshes.emplace_back();
    mesh.first_vertex = 0;
    mesh.first_index  = 0;
    mesh.vertex_count = 3;
    mesh.index_count  = 3;

    return model;
}

void load_material_textures(Mesh& mesh, Model& model, aiMaterial* mat, aiTextureType type)
{
    for (u32 i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        b32 skip = false;
        for (u32 j = 0; j < model.textures.size(); j++)
        {
            if (std::strcmp(model.textures[ j ].filename.data(), str.C_Str()) == 0)
            {
                mesh.material.base_color_texture = j;
                skip                             = true;
                break;
            }
        }

        if (!skip)
        {
            mesh.material.base_color_texture = model.textures.size();
            model.textures.emplace_back(model.directory, str.C_Str());
        }
    }
}

Mesh process_mesh(
    Model& model, const Matrix4& transform, VertexComponents vertex_components, aiMesh* mesh, const aiScene* scene)
{
    Mesh result;

    auto& vertices      = model.vertices;
    u32   vertices_size = vertices.size();

    result.first_vertex = vertices_size / model.vertex_stride;
    result.first_index  = model.indices.size();

    vertices.resize(vertices.size() + mesh->mNumVertices * model.vertex_stride);

    u32 v = vertices_size;

    for (u32 i = 0; i < mesh->mNumVertices; i++)
    {
        // positions
        Vector3 position(mesh->mVertices[ i ].x, mesh->mVertices[ i ].y, mesh->mVertices[ i ].z);
        position = transform * Vector4(position, 0.0);

        vertices[ v ]     = position.x;
        vertices[ v + 1 ] = position.y;
        vertices[ v + 2 ] = position.z;

        if (b32(vertex_components & VertexComponent::eNormal))
        {
            if (mesh->HasNormals())
            {
                Vector3 normal(mesh->mNormals[ i ].x, mesh->mNormals[ i ].y, mesh->mNormals[ i ].z);
                normal = Matrix3(glm::inverse(glm::transpose(transform))) * normal;

                vertices[ v + 3 ] = normal.x;
                vertices[ v + 4 ] = normal.y;
                vertices[ v + 5 ] = normal.z;
            }
        }

        if (b32(vertex_components & VertexComponent::eTexcoord))
        {
            if (mesh->mTextureCoords[ 0 ])
            {
                vertices[ v + 6 ] = mesh->mTextureCoords[ 0 ][ i ].x;
                vertices[ v + 7 ] = mesh->mTextureCoords[ 0 ][ i ].y;
            }
        }

        if (b32(vertex_components & VertexComponent::eTangent))
        {
            if (mesh->mTangents)
            {
                Vector3 tangent(mesh->mTangents[ i ].x, mesh->mTangents[ i ].y, mesh->mTangents[ i ].z);
                tangent = Matrix3(glm::inverse(glm::transpose(transform))) * tangent;

                vertices[ v + 8 ]  = mesh->mTangents[ i ].x;
                vertices[ v + 9 ]  = mesh->mTangents[ i ].y;
                vertices[ v + 10 ] = mesh->mTangents[ i ].z;
            }
        }

        v += model.vertex_stride;
    }

    // TODO: Count size
    for (u32 i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[ i ];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            model.indices.push_back(face.mIndices[ j ]);
    }

    // process materials
    aiMaterial* material = scene->mMaterials[ mesh->mMaterialIndex ];

    load_material_textures(result, model, material, aiTextureType_DIFFUSE);

    result.vertex_count = (model.vertices.size() / model.vertex_stride) - result.first_vertex;
    result.index_count  = model.indices.size() - result.first_index;

    return result;
}

void process_mesh_node(Model& model, VertexComponents vertex_components, aiNode* node, const aiScene* scene)
{
    auto& meshes = model.meshes;

    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[ node->mMeshes[ i ] ];
        auto&   t    = node->mTransformation;
        meshes.push_back(process_mesh(
            model,
            Matrix4(t.a1, t.b1, t.c1, t.d1, t.a2, t.b2, t.c2, t.d2, t.a3, t.b3, t.c3, t.d3, t.a4, t.b4, t.c4, t.d4),
            vertex_components, mesh, scene));
    }

    for (u32 i = 0; i < node->mNumChildren; ++i)
    {
        process_mesh_node(model, vertex_components, node->mChildren[ i ], scene);
    }
}

u32 count_stride(VertexComponents components)
{
    u32 stride = 3;

    if (b32(components & VertexComponents::eNormal))
    {
        stride += 3;
    }

    if (b32(components & VertexComponents::eTangent))
    {
        stride += 3;
    }

    if (b32(components & VertexComponents::eTexcoord))
    {
        stride += 2;
    }

    return stride;
}

void fill_vertex_layout(VertexLayout& layout, VertexComponents components)
{
    layout.binding_desc_count            = 1;
    layout.binding_descs[ 0 ].binding    = 0;
    layout.binding_descs[ 0 ].input_rate = VertexInputRate::eVertex;
    layout.binding_descs[ 0 ].stride     = 3 * sizeof(f32);
    layout.attribute_desc_count          = 1;
    layout.attribute_descs[ 0 ].binding  = 0;
    layout.attribute_descs[ 0 ].format   = Format::eR32G32B32Sfloat;
    layout.attribute_descs[ 0 ].location = 0;
    layout.attribute_descs[ 0 ].offset   = 0;

    u32 offset = 3 * sizeof(f32);

    if (b32(components & VertexComponents::eNormal))
    {
        layout.binding_descs[ 0 ].stride += 3 * sizeof(f32);

        u32 attrib_count = layout.attribute_desc_count;
        u32 location     = layout.attribute_descs[ attrib_count - 1 ].location + 1;
        // u32 offset       = layout.attribute_descs[ attrib_count - 1 ].offset + 3 * sizeof(f32);

        layout.attribute_desc_count++;
        layout.attribute_descs[ attrib_count ].binding  = 0;
        layout.attribute_descs[ attrib_count ].format   = Format::eR32G32B32Sfloat;
        layout.attribute_descs[ attrib_count ].location = location;
        layout.attribute_descs[ attrib_count ].offset   = offset;

        offset += 3 * sizeof(f32);
    }

    if (b32(components & VertexComponents::eTexcoord))
    {
        layout.binding_descs[ 0 ].stride += 2 * sizeof(f32);

        u32 attrib_count = layout.attribute_desc_count;
        u32 location     = layout.attribute_descs[ attrib_count - 1 ].location + 1;
        // u32 offset       = layout.attribute_descs[ attrib_count - 1 ].offset + 2 * sizeof(f32);

        layout.attribute_desc_count++;
        layout.attribute_descs[ attrib_count ].binding  = 0;
        layout.attribute_descs[ attrib_count ].format   = Format::eR32G32Sfloat;
        layout.attribute_descs[ attrib_count ].location = location;
        layout.attribute_descs[ attrib_count ].offset   = offset;

        offset += 2 * sizeof(f32);
    }

    if (b32(components & VertexComponents::eTangent))
    {
        layout.binding_descs[ 0 ].stride += 3 * sizeof(f32);

        u32 attrib_count = layout.attribute_desc_count;
        u32 location     = layout.attribute_descs[ attrib_count - 1 ].location + 1;

        layout.attribute_desc_count++;
        layout.attribute_descs[ attrib_count ].binding  = 0;
        layout.attribute_descs[ attrib_count ].format   = Format::eR32G32B32Sfloat;
        layout.attribute_descs[ attrib_count ].location = location;
        layout.attribute_descs[ attrib_count ].offset   = offset;

        offset += 3 * sizeof(f32);
    }
}

void load_model(Model& model, VertexComponents vertex_components, const std::string& filename)
{
    Assimp::Importer importer;
    const aiScene*   scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_CalcTangentSpace);

    FT_ASSERT(scene && (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) == 0 && scene->mRootNode && "Failed to load model");

    std::string model_directory = filename.substr(0, filename.find_last_of('/'));
    FT_TRACE("model directory {}", model_directory);

    model.directory     = model_directory;
    model.vertex_stride = count_stride(vertex_components);
    model.textures.emplace_back();
    fill_vertex_layout(model.vertex_layout, vertex_components);
    process_mesh_node(model, vertex_components, scene->mRootNode, scene);
}
} // namespace fluent
