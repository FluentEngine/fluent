#include "utils/model_loader.hpp"
#include "resource_manager/resource_manager.hpp"

namespace fluent
{
const Device*  ResourceManager::m_device       = nullptr;
CommandPool*   ResourceManager::m_command_pool = nullptr;
CommandBuffer* ResourceManager::m_cmd          = nullptr;
Queue*         ResourceManager::m_queue        = nullptr;

std::unordered_map<u32, Geometry*> ResourceManager::m_geometries;

void ResourceManager::create_staging_buffer(u32 size, Buffer** buffer)
{
    BufferDesc buffer_desc{};
    buffer_desc.descriptor_type = DescriptorType::eHostVisibleBuffer;
    buffer_desc.size            = size;
    create_buffer(m_device, &buffer_desc, buffer);
}

void ResourceManager::init(const Device* device)
{
    m_device = device;

    QueueDesc queue_desc{};
    queue_desc.queue_type = QueueType::eTransfer;
    create_queue(m_device, &queue_desc, &m_queue);

    CommandPoolDesc command_pool_desc{};
    command_pool_desc.queue = m_queue;
    create_command_pool(m_device, &command_pool_desc, &m_command_pool);

    create_command_buffers(m_device, m_command_pool, 1, &m_cmd);
}

void ResourceManager::shutdown()
{
    for (auto& [ id, geometry ] : m_geometries)
    {
        free_geometry(geometry);
    }
    destroy_command_buffers(m_device, m_command_pool, 1, &m_cmd);
    destroy_command_pool(m_device, m_command_pool);
    destroy_queue(m_queue);
}

void ResourceManager::load_buffer(const BufferLoadDesc* desc)
{
    FT_ASSERT(desc->p_buffer);

    Buffer* buffer = nullptr;

    if (*desc->p_buffer == nullptr)
    {
        create_buffer(m_device, &desc->buffer_desc, desc->p_buffer);
    }

    buffer = *desc->p_buffer;

    if (desc->data)
    {
        if (b32(desc->buffer_desc.descriptor_type & DescriptorType::eHostVisibleBuffer))
        {
            // TODO: Add asserts, check sizes
            map_memory(m_device, buffer);
            std::memcpy(( u8* ) buffer->mapped_memory + desc->offset, desc->data, desc->size);
            unmap_memory(m_device, buffer);
        }
        else
        {
            Buffer* staging_buffer = nullptr;
            create_staging_buffer(desc->size, &staging_buffer);
            map_memory(m_device, staging_buffer);
            std::memcpy(staging_buffer->mapped_memory, desc->data, desc->size);
            unmap_memory(m_device, staging_buffer);

            begin_command_buffer(m_cmd);
            cmd_copy_buffer(m_cmd, staging_buffer, 0, buffer, desc->offset, desc->size);
            end_command_buffer(m_cmd);

            immediate_submit(m_queue, m_cmd);
            destroy_buffer(m_device, staging_buffer);
        }
    }
}

void ResourceManager::load_geometry(const GeometryLoadDesc* desc)
{
    FT_ASSERT(desc);
    if (!desc->filename.empty())
    {
        // load geometry from file
        *desc->p_geometry  = new Geometry{};
        Geometry* geometry = *desc->p_geometry;

        ModelLoader  model_loader;
        GeometryData data = model_loader.load(desc);

        geometry->vertex_layout = data.vertex_layout;

        for (u32 i = 0; i < data.nodes.size(); ++i)
        {
            auto& geometry_node = geometry->nodes.emplace_back();

            BufferLoadDesc buffer_load_desc{};
            buffer_load_desc.p_buffer   = &geometry_node.vertex_buffer;
            buffer_load_desc.offset     = 0;
            buffer_load_desc.size       = data.nodes[ i ].positions.size() * sizeof(data.nodes[ i ].positions[ 0 ]);
            buffer_load_desc.data       = data.nodes[ i ].positions.data();
            BufferDesc& buffer_desc     = buffer_load_desc.buffer_desc;
            buffer_desc.descriptor_type = DescriptorType::eVertexBuffer;
            buffer_desc.size            = buffer_load_desc.size;

            load_buffer(&buffer_load_desc);

            buffer_load_desc.p_buffer   = &geometry_node.index_buffer;
            buffer_load_desc.size       = data.nodes[ i ].indices.size() * sizeof(data.nodes[ i ].indices[ 0 ]);
            buffer_load_desc.data       = data.nodes[ i ].indices.data();
            buffer_desc.descriptor_type = DescriptorType::eIndexBuffer;
            buffer_desc.size            = buffer_load_desc.size;

            load_buffer(&buffer_load_desc);

            geometry_node.index_count = data.nodes[ i ].indices.size();
        }

        u32 id = std::hash<std::string>{}(desc->filename);

        m_geometries[ id ] = geometry;
        geometry->id       = id;
    }
    else if (desc->data)
    {
        // TODO: Support loading from user data
    }
}

void ResourceManager::free_geometry(Geometry* geometry)
{
    m_geometries.erase(geometry->id);
    for (auto& node : geometry->nodes)
    {
        destroy_buffer(m_device, node.vertex_buffer);
        destroy_buffer(m_device, node.index_buffer);
    }

    delete geometry;
}
} // namespace fluent
