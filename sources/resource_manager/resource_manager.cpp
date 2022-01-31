#include "core/application.hpp"
#include "utils/model_loader.hpp"
#include "utils/image_loader.hpp"
#include "resource_manager/resource_manager.hpp"

namespace fluent
{
const Device*  ResourceManager::m_device       = nullptr;
CommandPool*   ResourceManager::m_command_pool = nullptr;
CommandBuffer* ResourceManager::m_cmd          = nullptr;
Queue*         ResourceManager::m_queue        = nullptr;

std::unordered_map<u32, Ref<Image>>    ResourceManager::m_images;
std::unordered_map<u32, Ref<Geometry>> ResourceManager::m_geometries;

void ResourceManager::create_staging_buffer(u32 size, BaseBuffer** buffer)
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
    m_images.clear();
    m_geometries.clear();
    destroy_command_buffers(m_device, m_command_pool, 1, &m_cmd);
    destroy_command_pool(m_device, m_command_pool);
    destroy_queue(m_queue);
}

void ResourceManager::load_buffer(Ref<Buffer>& buffer, const BufferLoadDesc* desc)
{
    FT_ASSERT(desc);

    if (buffer == nullptr)
    {
        buffer = Buffer::create(m_device, &desc->buffer_desc);
    }

    if (desc->data)
    {
        if (b32(desc->buffer_desc.descriptor_type & DescriptorType::eHostVisibleBuffer))
        {
            // TODO: Add asserts, check sizes
            map_memory(m_device, buffer->get());
            std::memcpy(( u8* ) buffer->get()->mapped_memory + desc->offset, desc->data, desc->size);
            unmap_memory(m_device, buffer->get());
        }
        else
        {
            BaseBuffer* staging_buffer = nullptr;
            create_staging_buffer(desc->size, &staging_buffer);
            map_memory(m_device, staging_buffer);
            std::memcpy(staging_buffer->mapped_memory, desc->data, desc->size);
            unmap_memory(m_device, staging_buffer);

            begin_command_buffer(m_cmd);
            cmd_copy_buffer(m_cmd, staging_buffer, 0, buffer->get(), desc->offset, desc->size);
            end_command_buffer(m_cmd);

            immediate_submit(m_queue, m_cmd);
            destroy_buffer(m_device, staging_buffer);
        }
    }
}

void ResourceManager::release_buffer(Ref<Buffer>& buffer)
{
    buffer = nullptr;
}

void ResourceManager::load_image(Ref<Image>& image, const ImageLoadDesc* desc)
{
    FT_ASSERT(desc);

    BaseBuffer* staging_buffer = nullptr;

    if (!desc->filename.empty())
    {
        u32  id = std::hash<std::string>{}(desc->filename);
        auto it = m_images.find(id);

        if (it != m_images.cend())
        {
            Ref<Image> r = m_images[ id ];
            image        = r;
        }
        else
        {
            std::string ext = desc->filename.substr(desc->filename.find_last_of("."));
            ImageDesc   image_desc{};
            u32         size = 0;
            void*       data = nullptr;

            if (ext == ".dds")
            {
                image_desc = read_dds_image(get_app_textures_directory() + desc->filename, desc->flip, &size, &data);
            }
            else
            {
                image_desc = read_image(get_app_textures_directory() + desc->filename, desc->flip, &size, &data);
            }

            image_desc.descriptor_type = DescriptorType::eSampledImage;

            image          = Image::create(m_device, &image_desc, id);
            m_images[ id ] = image;

            create_staging_buffer(size, &staging_buffer);
            map_memory(m_device, staging_buffer);
            std::memcpy(staging_buffer->mapped_memory, data, size);
            unmap_memory(m_device, staging_buffer);

            if (ext == ".dds")
            {
                release_dds_image_data(data);
            }
            else
            {
                release_image_data(data);
            }
        }
    }
    else if (desc->data)
    {
        if (image == nullptr)
        {
            image = Image::create(m_device, &desc->image_desc);

            m_images[ image->id() ] = image;
        }

        create_staging_buffer(desc->size, &staging_buffer);
        map_memory(m_device, staging_buffer);
        std::memcpy(staging_buffer->mapped_memory, desc->data, desc->size);
        unmap_memory(m_device, staging_buffer);
    }

    if (staging_buffer)
    {
        ImageBarrier image_barrier{};
        image_barrier.image     = image->get();
        image_barrier.src_queue = m_queue;
        image_barrier.dst_queue = m_queue;
        image_barrier.old_state = ResourceState::eUndefined;
        image_barrier.new_state = ResourceState::eTransferDst;

        begin_command_buffer(m_cmd);
        cmd_barrier(m_cmd, 0, nullptr, 1, &image_barrier);
        cmd_copy_buffer_to_image(m_cmd, staging_buffer, 0, image->get());
        image_barrier.old_state = ResourceState::eTransferDst;
        image_barrier.new_state = ResourceState::eShaderReadOnly;
        cmd_barrier(m_cmd, 0, nullptr, 1, &image_barrier);
        end_command_buffer(m_cmd);

        immediate_submit(m_queue, m_cmd);
        destroy_buffer(m_device, staging_buffer);
    }
}

void ResourceManager::release_image(Ref<Image>& image)
{
    image = nullptr;
}

void ResourceManager::load_geometry(Ref<Geometry>& geometry, const GeometryLoadDesc* desc)
{
    FT_ASSERT(desc);

    if (!desc->filename.empty())
    {
        // load geometry from file
        u32 id = std::hash<std::string>{}(desc->filename);

        auto it = m_geometries.find(id);

        if (it != m_geometries.cend())
        {
            // TODO: not so nice solution
            Ref<Geometry> r = it->second;
            geometry        = r;
        }
        else
        {
            ModelLoader  model_loader;
            GeometryDesc geometry_desc = model_loader.load(desc);

            geometry = Geometry::create(m_device, &geometry_desc, id);

            m_geometries[ id ] = geometry;
        }
    }
    else if (desc->desc)
    {
        // TODO: Support loading from user data
    }
}

void ResourceManager::release_geometry(Ref<Geometry>& geometry)
{
    geometry = nullptr;
}
} // namespace fluent