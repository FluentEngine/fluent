#include "tinyimageformat_query.h"
#include "core/application.hpp"
#include "utils/utils.hpp"
#include "renderer/resource_loader.hpp"

namespace fluent
{

struct ResourceLoader
{
    Queue         queue;
    CommandPool   cmd_pool;
    b32           cmd_record_in_process = false;
    CommandBuffer cmd;
    Fence         fence;
    StagingBuffer staging_buffer;
} resource_loader;

void init_resource_loader(Device const& device)
{
    QueueDesc queue_desc{};
    queue_desc.queue_type = QueueType::eTransfer;

    get_queue(device, queue_desc, resource_loader.queue);

    CommandPoolDesc cmd_pool_desc{};
    cmd_pool_desc.queue = &resource_loader.queue;

    resource_loader.cmd_pool = create_command_pool(device, cmd_pool_desc);

    allocate_command_buffers(device, resource_loader.cmd_pool, 1, &resource_loader.cmd);

    resource_loader.fence = create_fence(device);
    // create staging buffer
    BufferDesc buffer_desc{};
    buffer_desc.size            = STAGING_BUFFER_SIZE;
    buffer_desc.descriptor_type = DescriptorType::eHostVisibleBuffer | DescriptorType::eDeviceLocalBuffer;

    resource_loader.staging_buffer.memory_used = 0;
    resource_loader.staging_buffer.buffer      = create_buffer(device, buffer_desc);
    map_memory(device, resource_loader.staging_buffer.buffer);
}

void shutdown_resource_loader(Device const& device)
{
    queue_wait_idle(resource_loader.queue);
    destroy_buffer(device, resource_loader.staging_buffer.buffer);
    destroy_fence(device, resource_loader.fence);
    free_command_buffers(device, resource_loader.cmd_pool, 1, &resource_loader.cmd);
    destroy_command_pool(device, resource_loader.cmd_pool);
}

void resource_loader_begin_cmd()
{
    if (!resource_loader.cmd_record_in_process)
    {
        begin_command_buffer(resource_loader.cmd);
        resource_loader.cmd_record_in_process = true;
    }
}

void resource_loader_end_cmd()
{
    end_command_buffer(resource_loader.cmd);
    resource_loader.cmd_record_in_process = false;
}

void resource_loader_wait_idle()
{
    if (resource_loader.cmd_record_in_process)
    {
        resource_loader_end_cmd();
        nolock_submit(resource_loader.queue, resource_loader.cmd, nullptr);
        queue_wait_idle(resource_loader.queue);
        resource_loader.staging_buffer.memory_used = 0;
    }
}

void write_to_staging_buffer(const Device& device, u32 size, const void* data)
{
    // TODO: decide what to do if no space left
    FT_ASSERT(size < (resource_loader.staging_buffer.buffer.size - resource_loader.staging_buffer.memory_used));
    u8* dst = ( u8* ) resource_loader.staging_buffer.buffer.mapped_memory + resource_loader.staging_buffer.memory_used;
    std::memcpy(dst, data, size);
    resource_loader.staging_buffer.memory_used += size;
}

Shader load_shader(const Device& device, const std::string& filename)
{
    ShaderDesc shader_desc{};

    std::string stage_ext = filename.substr(filename.find_last_of("."));

    auto bytecode = read_file_binary(get_app_shaders_directory() + filename + ".glsl.spv");

    if (stage_ext == ".vert")
    {
        shader_desc.stage = ShaderStage::eVertex;
    }
    else if (stage_ext == ".frag")
    {
        shader_desc.stage = ShaderStage::eFragment;
    }
    else if (stage_ext == ".comp")
    {
        shader_desc.stage = ShaderStage::eCompute;
    }

    shader_desc.bytecode_size = bytecode.size() * sizeof(bytecode[ 0 ]);
    shader_desc.bytecode      = bytecode.data();

    Shader shader = create_shader(device, shader_desc);

    return shader;
}

void update_buffer(const Device& device, BufferUpdateDesc const& desc, UpdateFence* fence /*= nullptr */)
{
    FT_ASSERT(desc.buffer);
    FT_ASSERT(desc.offset + desc.size <= desc.buffer->size);
    FT_ASSERT(desc.data);

    Buffer* buffer = desc.buffer;

    if (b32(buffer->resource_state & ResourceState::eTransferSrc))
    {
        bool was_mapped = buffer->mapped_memory;
        if (!was_mapped)
        {
            map_memory(device, *buffer);
        }

        std::memcpy(( u8* ) buffer->mapped_memory + desc.offset, desc.data, desc.size);

        if (!was_mapped)
        {
            unmap_memory(device, *buffer);
        }
    }
    else
    {
        write_to_staging_buffer(device, desc.size, desc.data);

        resource_loader_begin_cmd();
        cmd_copy_buffer(
            resource_loader.cmd, resource_loader.staging_buffer.buffer,
            resource_loader.staging_buffer.memory_used - desc.size, *desc.buffer, desc.offset, desc.size);
    }
}

Image load_image_from_dds_file(const Device& device, const char* filename, ResourceState resource_state, b32 flip)
{
    std::string filepath = std::string(get_app_textures_directory()) + std::string(filename);

    void* data = nullptr;

    ImageDesc image_desc       = read_dds_image(filepath.c_str(), flip, &data);
    image_desc.descriptor_type = DescriptorType::eSampledImage;
    Image image                = create_image(device, image_desc);

    ImageUpdateDesc image_update_desc{};
    image_update_desc.data           = data;
    image_update_desc.image          = &image;
    image_update_desc.resource_state = ResourceState::eUndefined;
    image_update_desc.size           = image_desc.width * image_desc.height;

    // TODO: Remove it
    ImageBarrier image_barrier{};
    image_barrier.image     = &image;
    image_barrier.src_queue = &resource_loader.queue;
    image_barrier.dst_queue = &resource_loader.queue;
    image_barrier.old_state = ResourceState::eUndefined;
    image_barrier.new_state = ResourceState::eShaderReadOnly;

    update_image(device, image_update_desc);
    resource_loader_begin_cmd();
    cmd_barrier(resource_loader.cmd, 0, nullptr, 1, &image_barrier);

    release_dds_image(data);
    return image;
}

Image load_image_from_file(const Device& device, const char* filename, ResourceState resource_state, b32 flip)
{
    std::string filepath = std::string(get_app_textures_directory()) + std::string(filename);

    void* data = nullptr;

    ImageDesc image_desc       = read_image(filepath.c_str(), flip, &data);
    image_desc.descriptor_type = DescriptorType::eSampledImage;
    Image image                = create_image(device, image_desc);

    ImageUpdateDesc image_update_desc{};
    image_update_desc.data           = data;
    image_update_desc.image          = &image;
    image_update_desc.resource_state = ResourceState::eUndefined;
    image_update_desc.size =
        image_desc.width * image_desc.height * TinyImageFormat_ChannelCount(( TinyImageFormat ) image_desc.format);

    // TODO: Remove it
    ImageBarrier image_barrier;
    image_barrier.image     = &image;
    image_barrier.src_queue = &resource_loader.queue;
    image_barrier.dst_queue = &resource_loader.queue;
    image_barrier.old_state = ResourceState::eTransferDst;
    image_barrier.new_state = ResourceState::eShaderReadOnly;

    update_image(device, image_update_desc);
    resource_loader_begin_cmd();
    cmd_barrier(resource_loader.cmd, 0, nullptr, 1, &image_barrier);

    release_image(data);

    return image;
}

void update_image(const Device& device, ImageUpdateDesc const& desc, UpdateFence* fence /* = nullptr */)
{
    write_to_staging_buffer(device, desc.size, desc.data);

    ImageBarrier image_barrier{};
    image_barrier.image     = desc.image;
    image_barrier.src_queue = &resource_loader.queue;
    image_barrier.dst_queue = &resource_loader.queue;

    resource_loader_begin_cmd();

    if (desc.resource_state != ResourceState::eTransferDst)
    {
        image_barrier.old_state = desc.resource_state;
        image_barrier.new_state = ResourceState::eTransferDst;
        cmd_barrier(resource_loader.cmd, 0, nullptr, 1, &image_barrier);
    }

    cmd_copy_buffer_to_image(
        resource_loader.cmd, resource_loader.staging_buffer.buffer,
        resource_loader.staging_buffer.memory_used - desc.size, *desc.image);

    if (desc.resource_state != ResourceState::eTransferDst && desc.resource_state != ResourceState::eUndefined)
    {
        image_barrier.old_state = ResourceState::eTransferDst;
        image_barrier.new_state = desc.resource_state;
        cmd_barrier(resource_loader.cmd, 0, nullptr, 1, &image_barrier);
    }
}

} // namespace fluent
