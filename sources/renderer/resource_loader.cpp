#include "renderer/resource_loader.hpp"

namespace fluent
{
const Device*  ResourceLoader::m_device       = nullptr;
Queue*         ResourceLoader::m_queue        = nullptr;
CommandPool*   ResourceLoader::m_command_pool = nullptr;
CommandBuffer* ResourceLoader::m_cmd          = nullptr;

static b32 need_staging(Buffer* buffer)
{
    return !(buffer->memory_usage == MemoryUsage::eCpuOnly || buffer->memory_usage == MemoryUsage::eCpuToGpu);
}

void ResourceLoader::init(const Device* device)
{
    m_device = device;

    QueueDesc queue_desc{};
    queue_desc.queue_type = QueueType::eGraphics;
    create_queue(m_device, &queue_desc, &m_queue);

    CommandPoolDesc cmd_pool_desc{};
    cmd_pool_desc.queue = m_queue;
    create_command_pool(m_device, &cmd_pool_desc, &m_command_pool);

    create_command_buffers(m_device, m_command_pool, 1, &m_cmd);
}

void ResourceLoader::shutdown()
{
    destroy_command_buffers(m_device, m_command_pool, 1, &m_cmd);
    destroy_command_pool(m_device, m_command_pool);
    destroy_queue(m_queue);
}

Buffer* ResourceLoader::create_staging_buffer(u64 size, const void* data)
{
    BufferDesc desc{};
    desc.memory_usage = MemoryUsage::eCpuOnly;
    desc.size         = size;

    Buffer* staging_buffer = nullptr;
    create_buffer(m_device, &desc, &staging_buffer);
    map_memory(m_device, staging_buffer);
    std::memcpy(staging_buffer->mapped_memory, data, size);
    unmap_memory(m_device, staging_buffer);

    return staging_buffer;
}

void ResourceLoader::upload_buffer(Buffer* buffer, u64 offset, u64 size, const void* data)
{
    FT_ASSERT(buffer);
    FT_ASSERT(data);
    FT_ASSERT(size + offset <= buffer->size);

    if (need_staging(buffer))
    {
        Buffer* staging_buffer = create_staging_buffer(size, data);
        // TODO: merge uploads
        begin_command_buffer(m_cmd);
        cmd_copy_buffer(m_cmd, staging_buffer, 0, buffer, offset, size);
        end_command_buffer(m_cmd);
        immediate_submit(m_queue, m_cmd);

        destroy_buffer(m_device, staging_buffer);
    }
    else
    {
        map_memory(m_device, buffer);
        std::memcpy(( u8* ) buffer->mapped_memory + offset, data, size);
        unmap_memory(m_device, buffer);
    }
}

void* ResourceLoader::begin_upload_buffer(Buffer* buffer)
{
    FT_ASSERT(buffer);
    if (need_staging(buffer))
    {
        // TODO:
        return nullptr;
    }
    else
    {
        map_memory(m_device, buffer);
        return buffer->mapped_memory;
    }
}

void ResourceLoader::end_upload_buffer(Buffer* buffer)
{
    FT_ASSERT(buffer);
    if (need_staging(buffer))
    {
        // TODO:
    }
    else
    {
        unmap_memory(m_device, buffer);
    }
}

void ResourceLoader::upload_image(Image* image, u64 size, const void* data)
{
    Buffer* staging_buffer = create_staging_buffer(size, data);

    begin_command_buffer(m_cmd);
    cmd_copy_buffer_to_image(m_cmd, staging_buffer, 0, image);
    end_command_buffer(m_cmd);
    immediate_submit(m_queue, m_cmd);

    destroy_buffer(m_device, staging_buffer);
}

} // namespace fluent
