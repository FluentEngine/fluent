#pragma once

#include <vector>
#include "core/base.hpp"
#include "renderer/renderer_backend.hpp"

namespace fluent
{
class ResourceManager;

class Buffer
{
    friend class ResourceManager;

private:
    const Device* m_device = nullptr;
    BaseBuffer*   m_buffer = nullptr;
    u32           m_id     = 0;

    Buffer(const Device* device, const BufferDesc* desc, u32 id);

public:
    ~Buffer();

    operator BaseBuffer*() const
    {
        return m_buffer;
    }

    BaseBuffer* get() const
    {
        return m_buffer;
    }

    static Ref<Buffer> create(const Device* device, const BufferDesc* desc);
    static Ref<Buffer> create(const Device* device, const BufferDesc* desc, u32 id);
};

class Image
{
    friend class ResourceManager;

private:
    const Device* m_device = nullptr;
    BaseImage*    m_image  = nullptr;
    u32           m_id     = 0;

    Image(const Device* device, const ImageDesc* desc, u32 id);

public:
    ~Image();

    operator BaseImage*() const
    {
        return m_image;
    }

    BaseImage* get() const
    {
        return m_image;
    }

    static Ref<Image> create(const Device* device, const ImageDesc* desc);
    static Ref<Image> create(const Device* device, const ImageDesc* desc, u32 id);
};

struct GeometryDesc
{
    struct GeometryDataNode
    {
        std::vector<f32> positions;
        std::vector<u32> indices;
    };

    std::vector<GeometryDataNode> nodes;
    VertexLayout                  vertex_layout;
};

class Geometry
{
    friend class ResourceManager;

public:
    struct GeometryNode
    {
        Ref<Buffer> vertex_buffer;
        Ref<Buffer> index_buffer;
        u32         index_count;
    };

private:
    const Device* m_device = nullptr;
    u32           m_id     = 0;

    VertexLayout              m_vertex_layout;
    std::vector<GeometryNode> m_nodes;

    Geometry(const Device* device, const GeometryDesc* desc, u32 id);

public:
    ~Geometry();

    const VertexLayout& vertex_layout() const
    {
        return m_vertex_layout;
    }

    const std::vector<GeometryNode>& nodes() const
    {
        return m_nodes;
    }

    static Ref<Geometry> create(const Device* device, const GeometryDesc* desc);
    static Ref<Geometry> create(const Device* device, const GeometryDesc* desc, u32 id);
};
} // namespace fluent
