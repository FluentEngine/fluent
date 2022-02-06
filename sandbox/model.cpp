#include "model.hpp"

namespace fluent
{
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

    model.vertices = { -0.5, -0.5, 0.0, 0.5, -0.5, 0.0, 0.0, 0.5, 0.0 };
    model.indices  = { 0, 1, 2 };

    Mesh& mesh         = model.meshes.emplace_back();
    mesh.vertex_offset = 0;
    mesh.index_offset  = 0;
    mesh.vertex_count  = 9;
    mesh.index_count   = 3;
    mesh.vertex_stride = 3;

    return model;
}
} // namespace fluent
