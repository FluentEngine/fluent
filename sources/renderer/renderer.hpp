#pragma once

#include <mutex>
#include "volk.h"
#include "vk_mem_alloc.h"
#include "core/base.hpp"
#include "math/math.hpp"
#include "renderer/renderer_enums.hpp"
#include "renderer/shader_reflection.hpp"

namespace fluent
{
// Forward declares
struct Window;
struct RenderPass;
struct Buffer;
struct StagingBuffer;
struct Image;

static constexpr u32 STAGING_BUFFER_SIZE          = 10 * 1024 * 1024 * 8; // ~50 mb
static constexpr u32 FLUENT_VULKAN_API_VERSION    = VK_API_VERSION_1_2;
static constexpr u32 MAX_ATTACHMENTS_COUNT        = 10;
static constexpr u32 MAX_PUSH_CONSTANT_RANGE      = 128;
static constexpr u32 MAX_STAGE_COUNT              = 5;
static constexpr u32 MAX_VERTEX_BINDING_COUNT     = 15;
static constexpr u32 MAX_VERTEX_ATTRIBUTE_COUNT   = 15;
static constexpr u32 MAX_DESCRIPTOR_BINDING_COUNT = 15;

struct QueueDesc
{
    QueueType queue_type;
};

struct Queue
{
    u32                family_index;
    QueueType          type;
    VkQueue            queue;
    mutable std::mutex submit_mutex;
};

struct Semaphore
{
    VkSemaphore semaphore;
};

struct Fence
{
    VkFence fence = VK_NULL_HANDLE;
};

struct SwapchainDesc
{
    Queue* queue;
    u32    width;
    u32    height;
    u32    min_image_count;
    b32    builtin_depth;
};

struct SamplerDesc
{
    Filter             mag_filter     = Filter::eNearest;
    Filter             min_filter     = Filter::eNearest;
    SamplerMipmapMode  mipmap_mode    = SamplerMipmapMode::eNearest;
    SamplerAddressMode address_mode_u = SamplerAddressMode::eRepeat;
    SamplerAddressMode address_mode_v = SamplerAddressMode::eRepeat;
    SamplerAddressMode address_mode_w = SamplerAddressMode::eRepeat;
    f32                mip_lod_bias;
    bool               anisotropy_enable;
    f32                max_anisotropy;
    b32                compare_enable;
    CompareOp          compare_op = CompareOp::eNever;
    f32                min_lod;
    f32                max_lod;
};

struct Sampler
{
    VkSampler sampler;
};

struct ImageDesc
{
    u32            width;
    u32            height;
    u32            depth;
    Format         format;
    SampleCount    sample_count;
    u32            layer_count = 1;
    u32            mip_levels  = 1;
    DescriptorType descriptor_type;
};

struct Image
{
    VkImage        image;
    VkImageView    image_view;
    VmaAllocation  allocation;
    u32            width;
    u32            height;
    Format         format;
    SampleCount    sample_count;
    u32            mip_level_count = 1;
    u32            layer_count     = 1;
    DescriptorType descriptor_type;
};

struct BufferDesc
{
    u32            size;
    DescriptorType descriptor_type;
};

struct Buffer
{
    VkBuffer       buffer;
    VmaAllocation  allocation;
    u32            size;
    ResourceState  resource_state;
    DescriptorType descriptor_type;
    void*          mapped_memory = nullptr;
};

struct StagingBuffer
{
    Buffer buffer;
    u32    memory_used;
};

struct Swapchain
{
    VkPresentModeKHR              present_mode;
    VkColorSpaceKHR               color_space;
    VkSurfaceTransformFlagBitsKHR pre_transform;
    u32                           min_image_count;
    u32                           image_count;
    u32                           width;
    u32                           height;
    VkSurfaceKHR                  surface;
    VkSwapchainKHR                swapchain;
    Format                        format;
    Image*                        images;
    Image                         depth_image;
    RenderPass*                   render_passes;
    Queue*                        queue;
};

struct CommandPoolDesc
{
    Queue* queue;
};

struct CommandPool
{
    Queue*        queue;
    VkCommandPool command_pool;
};

struct CommandBuffer
{
    Queue*          queue;
    VkCommandBuffer command_buffer;
};

struct QueueSubmitDesc
{
    u32                  wait_semaphore_count;
    Semaphore*           wait_semaphores;
    u32                  command_buffer_count;
    const CommandBuffer* command_buffers;
    u32                  signal_semaphore_count;
    Semaphore*           signal_semaphores;
    Fence*               signal_fence;
};

struct QueuePresentDesc
{
    u32        wait_semaphore_count;
    Semaphore* wait_semaphores;
    Swapchain* swapchain;
    u32        image_index;
};

struct RenderPassDesc
{
    u32              width;
    u32              height;
    u32              color_attachment_count;
    Image*           color_attachments[ MAX_ATTACHMENTS_COUNT ];
    AttachmentLoadOp color_attachment_load_ops[ MAX_ATTACHMENTS_COUNT ];
    ResourceState    color_image_states[ MAX_ATTACHMENTS_COUNT ];
    Image*           depth_stencil;
    AttachmentLoadOp depth_stencil_load_op;
    ResourceState    depth_stencil_state;
};

struct RenderPass
{
    VkRenderPass  render_pass;
    VkFramebuffer framebuffer;
    u32           width;
    u32           height;
    u32           color_attachment_count;
    b32           has_depth_stencil;
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
    ClearValue        clear_values[ MAX_ATTACHMENTS_COUNT + 1 ];
};

struct BufferBarrier
{
    ResourceState old_state;
    ResourceState new_state;
    Buffer*       buffer;
    Queue*        src_queue;
    Queue*        dst_queue;
    u32           size;
    u32           offset;
};

struct ImageBarrier
{
    ResourceState old_state;
    ResourceState new_state;
    const Image*  image;
    const Queue*  src_queue;
    const Queue*  dst_queue;
};

struct ShaderDesc
{
    ShaderStage stage;
    u32         bytecode_size;
    const u32*  bytecode;
};

struct Shader
{
    ShaderStage    stage;
    VkShaderModule shader;
    ReflectionData reflect_data;
};

struct DescriptorSetLayout
{
    u32                   shader_count;
    Shader*               shaders;
    VkDescriptorSetLayout descriptor_set_layout;
};

struct VertexBindingDesc
{
    uint32_t        binding;
    uint32_t        stride;
    VertexInputRate input_rate;
};

struct VertexAttributeDesc
{
    uint32_t location;
    uint32_t binding;
    Format   format;
    uint32_t offset;
};

struct RasterizerStateDesc
{
    CullMode    cull_mode;
    FrontFace   front_face;
    PolygonMode polygon_mode = PolygonMode::eFill;
};

struct DepthStateDesc
{
    b32       depth_test;
    b32       depth_write;
    CompareOp compare_op;
};

struct PipelineDesc
{
    u32                  binding_desc_count;
    VertexBindingDesc    binding_descs[ MAX_VERTEX_BINDING_COUNT ];
    u32                  attribute_desc_count;
    VertexAttributeDesc  attribute_descs[ MAX_VERTEX_ATTRIBUTE_COUNT ];
    RasterizerStateDesc  rasterizer_desc;
    DepthStateDesc       depth_state_desc;
    DescriptorSetLayout* descriptor_set_layout;
    RenderPass*          render_pass;
};

struct Pipeline
{
    PipelineType     type;
    VkPipelineLayout pipeline_layout;
    VkPipeline       pipeline;
};

struct RendererDesc
{
    VkAllocationCallbacks* vulkan_allocator;
};

struct Renderer
{
    VkAllocationCallbacks*   vulkan_allocator;
    VkInstance               instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice         physical_device;
};

struct DeviceDesc
{
    u32 frame_in_use_count;
};

struct Device
{
    VkAllocationCallbacks* vulkan_allocator;
    VkInstance             instance;
    VkPhysicalDevice       physical_device;
    VkDevice               logical_device;
    VmaAllocator           memory_allocator;
    VkDescriptorPool       descriptor_pool;
    Queue                  queue;
    CommandPool            command_pool;
    CommandBuffer          cmd;
};

struct DescriptorSetDesc
{
    DescriptorSetLayout* descriptor_set_layout;
};

struct DescriptorSet
{
    VkDescriptorSet descriptor_set;
};

struct DescriptorBufferDesc
{
    Buffer* buffer;
    u32     offset = 0;
    u32     range  = 0;
};

struct DescriptorImageDesc
{
    Sampler*      sampler;
    Image*        image;
    ResourceState resource_state;
};

struct DescriptorWriteDesc
{
    u32                   descriptor_count;
    u32                   binding;
    DescriptorType        descriptor_type;
    DescriptorImageDesc*  descriptor_image_descs;
    DescriptorBufferDesc* descriptor_buffer_descs;
};

struct UiDesc
{
    const Window*     window;
    const Renderer*   renderer;
    const Device*     device;
    const Queue*      queue;
    const RenderPass* render_pass;
    u32               min_image_count;
    u32               image_count;
    u32               in_fly_frame_count;
    b32               docking   = false;
    b32               viewports = false;
};

struct UiContext
{
    VkDescriptorPool desriptor_pool;
};

Renderer create_renderer(const RendererDesc& desc);
void     destroy_renderer(Renderer& renderer);

void create_device(const Renderer& renderer, const DeviceDesc& desc, Device& device);
void destroy_device(Device& device);
void device_wait_idle(const Device& device);

void get_queue(const Device& device, const QueueDesc& desc, Queue& queue);
void queue_wait_idle(const Queue& queue);
void queue_submit(const Queue& queue, const QueueSubmitDesc& desc);
void queue_present(const Queue& queue, const QueuePresentDesc& desc);

Semaphore create_semaphore(const Device& device);
void      destroy_semaphore(const Device& device, Semaphore& semaphore);
Fence     create_fence(const Device& device);
void      destroy_fence(const Device& device, Fence& fence);
void      wait_for_fences(const Device& device, u32 count, Fence* fences);
void      reset_fences(const Device& device, u32 count, Fence* fences);

Swapchain         create_swapchain(const Device& device, const SwapchainDesc& desc);
void              resize_swapchain(const Device& device, Swapchain& swapchain, u32 width, u32 height);
void              destroy_swapchain(const Device& device, Swapchain& swapchain);
const RenderPass* get_swapchain_render_pass(const Swapchain& swapchain, u32 image_index);

CommandPool create_command_pool(const Device& device, const CommandPoolDesc& desc);
void        destroy_command_pool(const Device& device, CommandPool& command_pool);

void allocate_command_buffers(
    const Device& device, const CommandPool& command_pool, u32 count, CommandBuffer* command_buffers);
void free_command_buffers(
    const Device& device, const CommandPool& command_pool, u32 count, CommandBuffer* command_buffers);
void begin_command_buffer(const CommandBuffer& cmd);
void end_command_buffer(const CommandBuffer& cmd);

void acquire_next_image(
    const Device& device, const Swapchain& swapchain, const Semaphore& semaphore, const Fence& fence, u32& image_index);

RenderPass create_render_pass(const Device& device, const RenderPassDesc& desc);
void       update_render_pass(const Device& device, RenderPass& render_pass, const RenderPassDesc& desc);
void       destroy_render_pass(const Device& device, RenderPass& render_pass);

Shader create_shader(const Device& device, ShaderDesc& desc);
void   destroy_shader(const Device& device, Shader& shader);

DescriptorSetLayout create_descriptor_set_layout(const Device& device, u32 shader_count, Shader* shaders);
void                destroy_descriptor_set_layout(const Device& device, DescriptorSetLayout& layout);

Pipeline create_compute_pipeline(const Device& device, const PipelineDesc& desc);
Pipeline create_graphics_pipeline(const Device& device, const PipelineDesc& desc);
void     destroy_pipeline(const Device& device, Pipeline& pipeline);

void cmd_begin_render_pass(const CommandBuffer& cmd, const RenderPassBeginDesc& desc);
void cmd_end_render_pass(const CommandBuffer& cmd);

void cmd_barrier(
    const CommandBuffer& cmd, u32 buffer_barriers_count, const BufferBarrier* buffer_barriers, u32 image_barriers_count,
    const ImageBarrier* image_barriers);

void cmd_set_scissor(const CommandBuffer& cmd, i32 x, i32 y, u32 width, u32 height);
void cmd_set_viewport(const CommandBuffer& cmd, f32 x, f32 y, f32 width, f32 height, f32 min_depth, f32 max_depth);

void cmd_bind_pipeline(const CommandBuffer& cmd, const Pipeline& pipeline);
void cmd_draw(const CommandBuffer& cmd, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);
void cmd_draw_indexed(
    const CommandBuffer& cmd, u32 index_count, u32 instance_count, u32 first_index, u32 vertex_offset,
    u32 first_instance);

void cmd_bind_vertex_buffer(const CommandBuffer& cmd, const Buffer& buffer);
void cmd_bind_index_buffer_u32(const CommandBuffer& cmd, const Buffer& buffer);

void cmd_copy_buffer(
    const CommandBuffer& cmd, const Buffer& src, u32 src_offset, Buffer& dst, u32 dst_offset, u32 size);

void cmd_copy_buffer_to_image(const CommandBuffer& cmd, const Buffer& src, u32 src_offset, Image& dst);
void cmd_bind_descriptor_set(const CommandBuffer& cmd, const DescriptorSet& set, const Pipeline& pipeline);

void cmd_dispatch(const CommandBuffer& cmd, u32 group_count_x, u32 group_count_y, u32 group_count_z);

void cmd_push_constants(const CommandBuffer& cmd, const Pipeline& pipeline, u32 offset, u32 size, const void* data);

void cmd_blit_image(
    const CommandBuffer& cmd, const Image& src, ResourceState src_state, Image& dst, ResourceState dst_state,
    Filter filter);

void cmd_clear_color_image(const CommandBuffer& cmd, Image& image, Vector4 color);

void nolock_submit(const Queue& queue, const CommandBuffer& cmd, Fence* fence);

Buffer create_buffer(const Device& device, const BufferDesc& desc);
void   destroy_buffer(const Device& device, Buffer& buffer);

void map_memory(const Device& device, Buffer& buffer);
void unmap_memory(const Device& device, Buffer& buffer);

Sampler create_sampler(const Device& device, const SamplerDesc& desc);
void    destroy_sampler(const Device& device, Sampler& sampler);

Image create_image(const Device& device, const ImageDesc& desc);
void  destroy_image(const Device& device, Image& image);

DescriptorSet create_descriptor_set(const Device& device, const DescriptorSetDesc& desc);
void          destroy_descriptor_set(const Device& device, DescriptorSet& set);
void update_descriptor_set(const Device& device, DescriptorSet& set, u32 count, const DescriptorWriteDesc* descs);

UiContext create_ui_context(const UiDesc& desc);
void      destroy_ui_context(const Device& device, const UiContext& context);
void      ui_begin_frame();
void      ui_end_frame(const CommandBuffer& cmd);

} // namespace fluent