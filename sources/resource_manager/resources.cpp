#include "resource_manager/resource_manager.hpp"
#include "resource_manager/resources.hpp"

namespace fluent
{

Buffer::Buffer(const Device* device, const BufferDesc* desc, u32 id) : m_id(id), m_device(device)
{
    create_buffer(m_device, desc, &m_buffer);
}

Buffer::~Buffer()
{
    destroy_buffer(m_device, m_buffer);
}

Ref<Buffer> Buffer::create(const Device* device, const BufferDesc* desc)
{
    Buffer* buffer = new Buffer(device, desc, 0);
    buffer->m_id   = std::hash<Buffer*>{}(buffer);
    return Ref<Buffer>(buffer);
}

Ref<Buffer> Buffer::create(const Device* device, const BufferDesc* desc, u32 id)
{
    Buffer* buffer = new Buffer(device, desc, id);
    return Ref<Buffer>(buffer);
}

Image::Image(const Device* device, const ImageDesc* desc, u32 id) : m_id(id), m_device(device)
{
    create_image(m_device, desc, &m_image);
}

Image::~Image()
{
    destroy_image(m_device, m_image);
}

Ref<Image> Image::create(const Device* device, const ImageDesc* desc)
{
    Image* image = new Image(device, desc, 0);
    image->m_id  = std::hash<Image*>{}(image);
    return Ref<Image>(image);
}

Ref<Image> Image::create(const Device* device, const ImageDesc* desc, u32 id)
{
    Image* image = new Image(device, desc, id);
    return Ref<Image>(image);
}

Geometry::Geometry(const Device* device, const GeometryDesc* desc, u32 id)
{
    m_vertex_layout = desc->vertex_layout;

    for (u32 i = 0; i < desc->nodes.size(); ++i)
    {
        auto& geometry_node = m_nodes.emplace_back();

        BufferLoadDesc buffer_load_desc{};
        buffer_load_desc.offset     = 0;
        buffer_load_desc.size       = desc->nodes[ i ].positions.size() * sizeof(desc->nodes[ i ].positions[ 0 ]);
        buffer_load_desc.data       = desc->nodes[ i ].positions.data();
        BufferDesc& buffer_desc     = buffer_load_desc.buffer_desc;
        buffer_desc.descriptor_type = DescriptorType::eVertexBuffer;
        buffer_desc.size            = buffer_load_desc.size;

        ResourceManager::load_buffer(geometry_node.vertex_buffer, &buffer_load_desc);

        buffer_load_desc.size       = desc->nodes[ i ].indices.size() * sizeof(desc->nodes[ i ].indices[ 0 ]);
        buffer_load_desc.data       = desc->nodes[ i ].indices.data();
        buffer_desc.descriptor_type = DescriptorType::eIndexBuffer;
        buffer_desc.size            = buffer_load_desc.size;

        ResourceManager::load_buffer(geometry_node.index_buffer, &buffer_load_desc);

        geometry_node.index_count = desc->nodes[ i ].indices.size();
        geometry_node.transform   = desc->nodes[ i ].transform;
    }
}

Geometry::~Geometry()
{
}

Ref<Geometry> Geometry::create(const Device* device, const GeometryDesc* desc)
{
    Geometry* geometry = new Geometry(device, desc, 0);

    geometry->m_id = std::hash<Geometry*>{}(geometry);
    return Ref<Geometry>(geometry);
}

Ref<Geometry> Geometry::create(const Device* device, const GeometryDesc* desc, u32 id)
{
    Geometry* geometry = new Geometry(device, desc, id);
    return Ref<Geometry>(geometry);
}
} // namespace fluent
