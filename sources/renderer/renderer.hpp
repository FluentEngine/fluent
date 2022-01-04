#pragma once

#include "volk.h"
#include "vk_mem_alloc.h"
#include "core/base.hpp"
#include "renderer/renderer_enums.hpp"
#include "renderer/shader_reflection.hpp"

namespace fluent
{
// Forward declares
struct RenderPass;
struct Buffer;
struct StagingBuffer;
struct Image;

static constexpr u32 STAGING_BUFFER_SIZE = 10 * 1024 * 1024 * 8; // ~50 mb
static constexpr u32 FLUENT_VULKAN_API_VERSION = VK_API_VERSION_1_2;
static constexpr u32 MAX_ATTACHMENTS_COUNT = 10;
static constexpr u32 MAX_PUSH_CONSTANT_RANGE = 128;
static constexpr u32 MAX_STAGE_COUNT = 5;
static constexpr u32 MAX_VERTEX_BINGINGS_COUNT = 15;
static constexpr u32 MAX_VERTEX_ATTRIBUTE_COUNT = 15;

struct QueueDesc
{
    QueueType queue_type;
};

struct Queue
{
    u32 m_family_index;
    QueueType m_type;
    VkQueue m_queue;
};

struct Semaphore
{
    VkSemaphore m_semaphore;
};

struct Fence
{
    VkFence m_fence = VK_NULL_HANDLE;
};

struct SwapchainDesc
{
    Queue* queue;
    u32 width;
    u32 height;
    u32 image_count;
};

struct SamplerDesc
{
    Filter mag_filter = Filter::eNearest;
    Filter min_filter = Filter::eNearest;
    SamplerMipmapMode mipmap_mode = SamplerMipmapMode::eNearest;
    SamplerAddressMode address_mode_u = SamplerAddressMode::eRepeat;
    SamplerAddressMode address_mode_v = SamplerAddressMode::eRepeat;
    SamplerAddressMode address_mode_w = SamplerAddressMode::eRepeat;
    f32 mip_lod_bias;
    bool anisotropy_enable;
    f32 max_anisotropy;
    b32 compare_enable;
    CompareOp compare_op = CompareOp::eNever;
    f32 min_lod;
    f32 max_lod;
};

struct Sampler
{
    VkSampler m_sampler;
};

struct ImageDesc
{
    u32 width;
    u32 height;
    u32 depth;
    Format format;
    SampleCount sample_count;
    u32 layer_count = 1;
    u32 mip_levels = 1;
    ResourceState resource_state;
    DescriptorType descriptor_type;
    const char* filename;
    u32 data_size;
    void* data;
};

struct ImageUpdateDesc
{
    Image* image;
    u32 size;
    const void* data;
    ResourceState resource_state;
};

struct Image
{
    VkImage m_image;
    VkImageView m_image_view;
    VmaAllocation m_allocation;
    u32 m_width;
    u32 m_height;
    Format m_format;
    SampleCount m_sample_count;
    u32 m_mip_level_count = 1;
    u32 m_layer_count = 1;
    ResourceState m_resource_state;
    DescriptorType m_descriptor_type;
};

struct BufferDesc
{
    u32 size;
    ResourceState resource_state;
    DescriptorType descriptor_type;
    const void* data;
};

struct BufferUpdateDesc
{
    u32 offset;
    u32 size;
    const void* data;
    Buffer* buffer;
};

struct Buffer
{
    VkBuffer m_buffer;
    VmaAllocation m_allocation;
    u32 m_size;
    ResourceState m_resource_state;
    DescriptorType m_descriptor_type;
    void* m_mapped_memory;
};

struct StagingBuffer
{
    Buffer m_buffer;
    u32 m_memory_used;
};

struct Swapchain
{
    VkPresentModeKHR m_present_mode;
    u32 m_image_count;
    u32 m_width;
    u32 m_height;
    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapchain;
    Format m_format;
    Image* m_images;
    RenderPass* m_render_passes;
};

struct CommandPoolDesc
{
    Queue* queue;
};

struct CommandPool
{
    QueueType m_queue_type;
    VkCommandPool m_command_pool;
};

struct CommandBuffer
{
    QueueType m_queue_type;
    VkCommandBuffer m_command_buffer;
};

struct QueueSubmitDesc
{
    u32 wait_semaphore_count;
    Semaphore* wait_semaphores;
    u32 command_buffer_count;
    const CommandBuffer* command_buffers;
    u32 signal_semaphore_count;
    Semaphore* signal_semaphores;
    Fence* signal_fence;
};

struct QueuePresentDesc
{
    u32 wait_semaphore_count;
    Semaphore* wait_semaphores;
    Swapchain* swapchain;
    u32 image_index;
};

struct RenderPassDesc
{
    u32 width;
    u32 height;
    u32 color_attachment_count;
    Image* color_attachments[ MAX_ATTACHMENTS_COUNT ];
    AttachmentLoadOp color_attachment_load_ops[ MAX_ATTACHMENTS_COUNT ];
    ResourceState color_image_states[ MAX_ATTACHMENTS_COUNT ];
    Image* depth_stencil;
    AttachmentLoadOp depth_stencil_load_op;
    ResourceState depth_stencil_state;
};

struct RenderPass
{
    VkRenderPass m_render_pass;
    VkFramebuffer m_framebuffer;
    u32 m_width;
    u32 m_height;
    u32 m_color_attachment_count;
    b32 m_has_depth_stencil;
};

struct ClearValue
{
    f32 color[ 4 ];
    f32 depth;
    u32 stencil;
};

struct RenderPassBeginDesc
{
    const RenderPass* render_pass;
    ClearValue clear_values[ MAX_ATTACHMENTS_COUNT + 1 ];
};

struct BufferBarrier
{
    ResourceState old_state;
    ResourceState new_state;
    Buffer* buffer;
    Queue* src_queue;
    Queue* dst_queue;
    u32 size;
    u32 offset;
};

struct ImageBarrier
{
    ResourceState old_state;
    ResourceState new_state;
    Image* image;
    const Queue* src_queue;
    const Queue* dst_queue;
};

struct ShaderDesc
{
    ShaderStage stage;
    const char* filename;
};

struct Shader
{
    ShaderStage m_stage;
    VkShaderModule m_shader;
    ReflectionData m_reflect_data;
};

struct DescriptorSetLayoutDesc
{
    u32 m_shader_count;
    Shader* m_shaders;
};

struct DescriptorSetLayout
{
    u32 m_shader_count;
    Shader* m_shaders;
    VkDescriptorSetLayout m_descriptor_set_layout;
};

struct VertexBindingDesc
{
    uint32_t binding;
    uint32_t stride;
    VertexInputRate input_rate;
};

struct VertexAttributeDesc
{
    uint32_t location;
    uint32_t binding;
    Format format;
    uint32_t offset;
};

struct RasterizerStateDesc
{
    CullMode cull_mode;
    FrontFace front_face;
};

struct DepthStateDesc
{
    b32 depth_test;
    b32 depth_write;
    CompareOp compare_op;
};

struct PipelineDesc
{
    u32 binding_desc_count;
    VertexBindingDesc binding_descs[ MAX_VERTEX_BINGINGS_COUNT ];
    u32 attribute_desc_count;
    VertexAttributeDesc attribute_descs[ MAX_VERTEX_ATTRIBUTE_COUNT ];
    RasterizerStateDesc rasterizer_desc;
    DepthStateDesc depth_state_desc;
    DescriptorSetLayout* descriptor_set_layout;
    RenderPass* render_pass;
};

struct Pipeline
{
    PipelineType m_type;
    VkPipelineLayout m_pipeline_layout;
    VkPipeline m_pipeline;
};

struct RendererDesc
{
    VkAllocationCallbacks* vulkan_allocator;
};

struct Renderer
{
    VkAllocationCallbacks* m_vulkan_allocator;
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debug_messenger;
    VkPhysicalDevice m_physical_device;
};

struct DeviceDesc
{
    u32 frame_in_use_count;
};

struct Device
{
    VkAllocationCallbacks* m_vulkan_allocator;
    VkInstance m_instance;
    VkPhysicalDevice m_physical_device;
    VkDevice m_logical_device;
    VmaAllocator m_memory_allocator;
    Queue m_upload_queue;
    CommandPool m_command_pool;
    CommandBuffer m_upload_command_buffer;
    StagingBuffer m_staging_buffer;
    VkDescriptorPool m_descriptor_pool;
};

struct DescriptorSetDesc
{
    DescriptorSetLayout* descriptor_set_layout;
};

struct DescriptorSet
{
    VkDescriptorSet m_descriptor_set;
};

struct BufferSetWrite
{
    Buffer* buffer;
    u32 offset = 0;
    u32 range = 0;
};

struct ImageSetWrite
{
    Sampler* sampler;
    Image* image;
};

struct DescriptorSetUpdateDesc
{
    DescriptorType descriptor_type;
    u32 binding;
    u32 buffer_set_write_count = 0;
    BufferSetWrite* buffer_set_writes;
    u32 image_set_write_count = 0;
    ImageSetWrite* image_set_writes;
};

Renderer create_renderer(const RendererDesc& desc);
void destroy_renderer(Renderer& renderer);

Device create_device(const Renderer& renderer, const DeviceDesc& desc);
void destroy_device(Device& device);
void device_wait_idle(const Device& device);

Queue get_queue(const Device& device, const QueueDesc& desc);
void queue_wait_idle(const Queue& queue);
void queue_submit(const Queue& queue, const QueueSubmitDesc& desc);
void queue_present(const Queue& queue, const QueuePresentDesc& desc);

Semaphore create_semaphore(const Device& device);
void destroy_semaphore(const Device& device, Semaphore& semaphore);
Fence create_fence(const Device& device);
void destroy_fence(const Device& device, Fence& fence);
void wait_for_fences(const Device& device, u32 count, Fence* fences);
void reset_fences(const Device& device, u32 count, Fence* fences);

Swapchain create_swapchain(const Renderer& renderer, const Device& device, const SwapchainDesc& desc);
void destroy_swapchain(const Device& device, Swapchain& swapchain);
const RenderPass* get_swapchain_render_pass(const Swapchain& swapchain, u32 image_index);

CommandPool create_command_pool(const Device& device, const CommandPoolDesc& desc);
void destroy_command_pool(const Device& device, CommandPool& command_pool);

void allocate_command_buffers(
    const Device& device, const CommandPool& command_pool, u32 count, CommandBuffer* command_buffers);
void free_command_buffers(
    const Device& device, const CommandPool& command_pool, u32 count, CommandBuffer* command_buffers);
void begin_command_buffer(const CommandBuffer& command_buffer);
void end_command_buffer(const CommandBuffer& command_buffer);

void acquire_next_image(
    const Device& device, const Swapchain& swapchain, const Semaphore& semaphore, const Fence& fence, u32& image_index);

RenderPass create_render_pass(const Device& device, const RenderPassDesc& desc);
void destroy_render_pass(const Device& device, RenderPass& render_pass);

Shader create_shader(const Device& device, const ShaderDesc& desc);
void destroy_shader(const Device& device, Shader& shader);

DescriptorSetLayout create_descriptor_set_layout(const Device& device, const DescriptorSetLayoutDesc& desc);
void destroy_descriptor_set_layout(const Device& device, DescriptorSetLayout& layout);

Pipeline create_graphics_pipeline(const Device& device, const PipelineDesc& desc);
void destroy_pipeline(const Device& device, Pipeline& pipeline);

void cmd_begin_render_pass(const CommandBuffer& command_buffer, const RenderPassBeginDesc& desc);
void cmd_end_render_pass(const CommandBuffer& command_buffer);

void cmd_barrier(
    const CommandBuffer& command_buffer, u32 buffer_barriers_count, const BufferBarrier* buffer_barriers,
    u32 image_barriers_count, const ImageBarrier* image_barriers);

void cmd_set_scissor(const CommandBuffer& command_buffer, i32 x, i32 y, u32 width, u32 height);
void cmd_set_viewport(
    const CommandBuffer& command_buffer, f32 x, f32 y, f32 width, f32 height, f32 min_depth, f32 max_depth);

void cmd_bind_pipeline(const CommandBuffer& command_buffer, const Pipeline& pipeline);
void cmd_draw(
    const CommandBuffer& command_buffer, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);

void cmd_bind_vertex_buffer(const CommandBuffer& command_buffer, const Buffer& buffer);

void cmd_copy_buffer(
    const CommandBuffer& command_buffer, const Buffer& src, u32 src_offset, Buffer& dst, u32 dst_offset, u32 size);

void immediate_submit(const Queue& queue, const CommandBuffer& command_buffer);

Buffer create_buffer(const Device& device, const BufferDesc& desc);
void destroy_buffer(const Device& device, Buffer& buffer);

void update_buffer(const Device& device, BufferUpdateDesc& buffer_update_desc);

Sampler create_sampler(const Device& device, const SamplerDesc& desc);
void destroy_sampler(const Device& device, Sampler& sampler);

Image create_image(const Device& device, const ImageDesc& desc);
void destroy_image(const Device& device, Image& image);
Image load_image_from_dds_file(const Device& device, const char* filename, ResourceState resource_state);
Image load_image_from_file(const Device& device, const char* filename, ResourceState resource_state);

void update_image(const Device& device, const ImageUpdateDesc& desc);

DescriptorSet create_descriptor_set(const Device& device, const DescriptorSetDesc& desc);
void destroy_descriptor_set(const Device& device, DescriptorSet& set);
void update_descriptor_set(const Device& device, DescriptorSet& set, u32 count, const DescriptorSetUpdateDesc* descs);
void cmd_bind_descriptor_set(const CommandBuffer& command_buffer, const DescriptorSet& set, const Pipeline& pipeline);

} // namespace fluent