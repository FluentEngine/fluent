#include "tinyimageformat_query.h"
#include "core/application.hpp"
#include "utils/utils.hpp"
#include "renderer/resource_loader.hpp"

namespace fluent
{

static constexpr b32 single_threaded = true;

struct UpdateRequest
{
    Buffer staging_buffer;
};

struct ResourceLoader
{
    Device const* device = nullptr;
    Queue         queue;
    CommandPool   cmd_pool;
    u32           cmd_pending_record_count = 0;
    CommandBuffer cmd;
    Fence         fence;
    Buffer        staging_buffer;

    std::vector<UpdateRequest> update_requests;
} resource_loader;

void init_resource_loader(Device const& device)
{
    resource_loader.device = &device;

    QueueDesc queue_desc{};
    queue_desc.queue_type = QueueType::eTransfer;

    get_queue(device, queue_desc, resource_loader.queue);

    CommandPoolDesc cmd_pool_desc{};
    cmd_pool_desc.queue = &resource_loader.queue;

    resource_loader.cmd_pool = create_command_pool(device, cmd_pool_desc);

    allocate_command_buffers(device, resource_loader.cmd_pool, 1, &resource_loader.cmd);

    resource_loader.fence = create_fence(device);
}

void shutdown_resource_loader(Device const& device)
{
    queue_wait_idle(resource_loader.queue);
    destroy_fence(device, resource_loader.fence);
    free_command_buffers(device, resource_loader.cmd_pool, 1, &resource_loader.cmd);
    destroy_command_pool(device, resource_loader.cmd_pool);
}

CommandBuffer& acquire_cmd()
{
    if (resource_loader.cmd_pending_record_count == 0)
    {
        begin_command_buffer(resource_loader.cmd);
    }
    resource_loader.cmd_pending_record_count++;
    return resource_loader.cmd;
}

void resource_loader_wait_idle()
{
    queue_wait_idle(resource_loader.queue);
}

Buffer allocate_staging_buffer(const Device& device, u32 size)
{
    BufferDesc buffer_desc{};
    buffer_desc.size            = size;
    buffer_desc.descriptor_type = DescriptorType::eHostVisibleBuffer | DescriptorType::eDeviceLocalBuffer;

    Buffer buffer = create_buffer(device, buffer_desc);

    return buffer;
}

Buffer write_to_staging_buffer(const Device& device, u32 size, const void* data)
{
    FT_ASSERT(size > 0);
    FT_ASSERT(data);
    Buffer staging_buffer = allocate_staging_buffer(device, size);
    map_memory(device, staging_buffer);
    std::memcpy(staging_buffer.mapped_memory, data, size);
    unmap_memory(device, staging_buffer);

    return staging_buffer;
}

void request_update(UpdateRequest&& request)
{
    resource_loader.update_requests.emplace_back(request);
}

void update_thread_function()
{
    if (resource_loader.cmd_pending_record_count > 0)
    {
        resource_loader.cmd_pending_record_count--;
    }

    if (resource_loader.cmd_pending_record_count == 0)
    {
        end_command_buffer(resource_loader.cmd);
        nolock_submit(resource_loader.queue, resource_loader.cmd, nullptr);
        queue_wait_idle(resource_loader.queue);
        for (auto& request : resource_loader.update_requests)
        {
            destroy_buffer(*resource_loader.device, request.staging_buffer);
        }
        resource_loader.update_requests.clear();
    }
};

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
        auto& cmd = acquire_cmd();

        UpdateRequest request{};
        request.staging_buffer = write_to_staging_buffer(device, desc.size, desc.data);

        cmd_copy_buffer(cmd, request.staging_buffer, 0, *desc.buffer, desc.offset, desc.size);
        request_update(std::move(request));

        if (single_threaded)
        {
            update_thread_function();
        }
    }
}

Image load_image_from_dds_file(const Device& device, const char* filename, ResourceState resource_state, b32 flip)
{
    auto& cmd = acquire_cmd();

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

    cmd_barrier(cmd, 0, nullptr, 1, &image_barrier);

    release_dds_image(data);

    if (single_threaded)
    {
        update_thread_function();
    }

    return image;
}

Image load_image_from_file(const Device& device, const char* filename, ResourceState resource_state, b32 flip)
{
    auto& cmd = acquire_cmd();

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

    cmd_barrier(cmd, 0, nullptr, 1, &image_barrier);

    release_image(data);

    if (single_threaded)
    {
        update_thread_function();
    }

    return image;
}

void update_image(const Device& device, ImageUpdateDesc const& desc, UpdateFence* fence /* = nullptr */)
{
    auto& cmd = acquire_cmd();

    ImageBarrier image_barrier{};
    image_barrier.image     = desc.image;
    image_barrier.src_queue = &resource_loader.queue;
    image_barrier.dst_queue = &resource_loader.queue;

    if (desc.resource_state != ResourceState::eTransferDst)
    {
        image_barrier.old_state = desc.resource_state;
        image_barrier.new_state = ResourceState::eTransferDst;
        cmd_barrier(cmd, 0, nullptr, 1, &image_barrier);
    }

    UpdateRequest request{};
    request.staging_buffer = write_to_staging_buffer(device, desc.size, desc.data);
    cmd_copy_buffer_to_image(resource_loader.cmd, request.staging_buffer, 0, *desc.image);

    if (desc.resource_state != ResourceState::eTransferDst && desc.resource_state != ResourceState::eUndefined)
    {
        image_barrier.old_state = ResourceState::eTransferDst;
        image_barrier.new_state = desc.resource_state;
        cmd_barrier(resource_loader.cmd, 0, nullptr, 1, &image_barrier);
    }

    request_update(std::move(request));

    if (single_threaded)
    {
        update_thread_function();
    }
}

} // namespace fluent
