#pragma once

#include <vector>
#include "core/base.hpp"
#include "renderer/renderer.hpp"

namespace fluent
{
struct Texture
{
    ResourceState state;
    Image*        image;
};

struct GeometryData
{
    struct GeometryDataNode
    {
        std::vector<f32> positions;
        std::vector<u32> indices;
    };

    std::vector<GeometryDataNode> nodes;
    VertexLayout                  vertex_layout;
};

struct Geometry
{
    u32 id = 0;

    struct GeometryNode
    {
        Buffer* vertex_buffer;
        Buffer* index_buffer;
        u32     index_count;
    };

    VertexLayout              vertex_layout;
    std::vector<GeometryNode> nodes;
};
} // namespace fluent
