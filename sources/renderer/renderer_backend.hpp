#pragma once

#include <mutex>
#ifdef VULKAN_BACKEND
#include <volk.h>
#include <vk_mem_alloc.h>
#endif
#ifdef D3D12_BACKEND
#include <dxgi1_4.h>
#include <d3d12.h>
#endif
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
struct Queue;
struct CommandBuffer;
struct CommandPool;

static constexpr u32 MAX_ATTACHMENTS_COUNT        = 10;
static constexpr u32 MAX_PUSH_CONSTANT_RANGE      = 128;
static constexpr u32 MAX_STAGE_COUNT              = 5;
static constexpr u32 MAX_VERTEX_BINDING_COUNT     = 15;
static constexpr u32 MAX_VERTEX_ATTRIBUTE_COUNT   = 15;
static constexpr u32 MAX_DESCRIPTOR_BINDING_COUNT = 15;
static constexpr u32 MAX_SET_COUNT                = 10;

struct RendererBackendDesc
{
#ifdef VULKAN_BACKEND
    struct
    {
        u32                    api_version = VK_API_VERSION_1_2;
        VkAllocationCallbacks* vulkan_allocator;
    } p;
#endif
};

struct RendererBackend
{
#ifdef VULKAN_BACKEND
    struct
    {
        VkAllocationCallbacks*   vulkan_allocator;
        VkInstance               instance;
        VkDebugUtilsMessengerEXT debug_messenger;
        VkPhysicalDevice         physical_device;
        u32                      api_version;
    } p;
#endif
#ifdef D3D12_BACKEND
    struct
    {
        IDXGIFactory4* factory;
    } p;
#endif
};

struct DeviceDesc
{
    u32 frame_in_use_count;
};

struct Device
{
#ifdef VULKAN_BACKEND
    struct
    {
        VkAllocationCallbacks* vulkan_allocator;
        VkInstance             instance;
        VkPhysicalDevice       physical_device;
        VkDevice               logical_device;
        VmaAllocator           memory_allocator;
        VkDescriptorPool       descriptor_pool;
    } p;
#endif
#ifdef D3D12_BACKEND
    struct
    {
        IDXGIFactory4*        factory;
        ID3D12Device*         device;
        ID3D12DescriptorHeap* rtv_heap;
    } p;
#endif
};

struct CommandPoolDesc
{
    Queue* queue;
};

struct CommandPool
{
    Queue* queue;
#ifdef VULKAN_BACKEND
    struct
    {
        VkCommandPool command_pool;
    } p;
#endif
#ifdef D3D12_BACKEND
    struct
    {
        ID3D12CommandAllocator* command_allocator;
    } p;
#endif
};

struct CommandBuffer
{
    Queue* queue;
#ifdef VULKAN_BACKEND
    struct
    {
        VkCommandBuffer command_buffer;
    } p;
#endif
#ifdef D3D12_BACKEND
    struct
    {
        ID3D12GraphicsCommandList* command_list;
    } p;
#endif
};

struct QueueDesc
{
    QueueType queue_type;
};

struct Queue
{
    u32       family_index;
    QueueType type;
#ifdef VULKAN_BACKEND
    struct
    {
        VkQueue queue;
    } p;
#endif
#ifdef D3D12_BACKEND
    struct
    {
        ID3D12CommandQueue* queue;
    } p;
#endif
};

struct Semaphore
{
#ifdef VULKAN_BACKEND
    struct
    {
        VkSemaphore semaphore;
    } p;
#endif
};

struct Fence
{
#ifdef VULKAN_BACKEND
    struct
    {
        VkFence fence = VK_NULL_HANDLE;
    } p;
#endif
#ifdef D3D12_BACKEND
    struct
    {
        ID3D12Fence* fence;
    } p;
#endif
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
#ifdef VULKAN_BACKEND
    struct
    {
        VkSampler sampler;
    } p;
#endif
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
    u32            width;
    u32            height;
    Format         format;
    SampleCount    sample_count;
    u32            mip_level_count = 1;
    u32            layer_count     = 1;
    DescriptorType descriptor_type;
#ifdef VULKAN_BACKEND
    struct
    {
        VkImage       image;
        VkImageView   image_view;
        VmaAllocation allocation;
    } p;
#endif
#ifdef D3D12_BACKEND
    struct
    {
        ID3D12Resource*             image;
        D3D12_CPU_DESCRIPTOR_HANDLE image_view;
    } p;
#endif
};

struct BufferDesc
{
    u64            size = 0;
    DescriptorType descriptor_type;
    MemoryUsage    memory_usage = MemoryUsage::eGpuOnly;
};

struct Buffer
{
    u64            size;
    ResourceState  resource_state;
    DescriptorType descriptor_type;
    MemoryUsage    memory_usage;
    void*          mapped_memory = nullptr;
#ifdef VULKAN_BACKEND
    struct
    {
        VkBuffer      buffer;
        VmaAllocation allocation;
    } p;
#endif
};

struct SwapchainDesc
{
    Queue* queue;
    u32    width;
    u32    height;
    Format format;
    b32    vsync;
    u32    min_image_count;
};

struct Swapchain
{
    u32     min_image_count;
    u32     image_count;
    u32     width;
    u32     height;
    Format  format;
    Image** images;
    Queue*  queue;
#ifdef VULKAN_BACKEND
    struct
    {
        VkPresentModeKHR              present_mode;
        VkColorSpaceKHR               color_space;
        VkSurfaceTransformFlagBitsKHR pre_transform;
        VkSurfaceKHR                  surface;
        VkSwapchainKHR                swapchain;
    } p;
#endif
#ifdef D3D12_BACKEND
    struct
    {
        IDXGISwapChain* swapchain;
    } p;
#endif
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
    Image*           resolve;
    AttachmentLoadOp resolve_load_op;
    ResourceState    resolve_state;
};

struct RenderPass
{
    u32         width;
    u32         height;
    SampleCount sample_count;
    u32         color_attachment_count;
    b32         has_depth_stencil;
#ifdef VULKAN_BACKEND
    struct
    {
        VkRenderPass  render_pass;
        VkFramebuffer framebuffer;
    } p;
#endif
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

// TODO:
struct MemoryBarrier
{
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
    ReflectionData reflect_data;
#ifdef VULKAN_BACKEND
    struct
    {
        VkShaderModule shader;
    } p;
#endif
};

struct DescriptorSetLayout
{
    u32      shader_count;
    Shader** shaders;
    u32      descriptor_set_layout_count = 0;
#ifdef VULKAN_BACKEND
    struct
    {
        VkDescriptorSetLayout descriptor_set_layouts[ MAX_SET_COUNT ];
    } p;
#endif
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

struct VertexLayout
{
    u32                 binding_desc_count;
    VertexBindingDesc   binding_descs[ MAX_VERTEX_BINDING_COUNT ];
    u32                 attribute_desc_count;
    VertexAttributeDesc attribute_descs[ MAX_VERTEX_ATTRIBUTE_COUNT ];
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
    VertexLayout         vertex_layout;
    RasterizerStateDesc  rasterizer_desc;
    PrimitiveTopology    topology = PrimitiveTopology::eTriangleList;
    DepthStateDesc       depth_state_desc;
    u32                  shader_count = 0;
    Shader*              shaders[ MAX_STAGE_COUNT ];
    DescriptorSetLayout* descriptor_set_layout;
    const RenderPass*    render_pass;
};

struct Pipeline
{
    PipelineType type;
#ifdef VULKAN_BACKEND
    struct
    {
        VkPipelineLayout pipeline_layout;
        VkPipeline       pipeline;
    } p;
#endif
};

struct DescriptorSetDesc
{
    u32                  set = 0;
    DescriptorSetLayout* descriptor_set_layout;
};

struct DescriptorSet
{
#ifdef VULKAN_BACKEND
    struct
    {
        VkDescriptorSet descriptor_set;
    } p;
#endif
};

struct BufferDescriptor
{
    Buffer* buffer;
    u64     offset = 0;
    u64     range  = 0;
};

struct ImageDescriptor
{
    Sampler*      sampler;
    Image*        image;
    ResourceState resource_state;
};

struct DescriptorWrite
{
    u32               descriptor_count;
    u32               binding;
    DescriptorType    descriptor_type;
    ImageDescriptor*  image_descriptors;
    BufferDescriptor* buffer_descriptors;
};

// TODO:
#ifdef VULKAN_BACKEND
using DrawIndexedIndirectCommand = VkDrawIndexedIndirectCommand;
#endif

struct UiDesc
{
    const Window*          window;
    const RendererBackend* backend;
    const Device*          device;
    const Queue*           queue;
    const RenderPass*      render_pass;
    u32                    min_image_count;
    u32                    image_count;
    u32                    in_fly_frame_count;
    b32                    docking   = false;
    b32                    viewports = false;
};

struct UiContext
{
#ifdef VULKAN_BACKEND
    struct
    {
        VkDescriptorPool desriptor_pool;
    } p;
#endif
};

void create_renderer_backend( const RendererBackendDesc* desc,
                              RendererBackend**          backend );

void destroy_renderer_backend( RendererBackend* backend );

void create_device( const RendererBackend* backend,
                    const DeviceDesc*      desc,
                    Device**               device );

void destroy_device( Device* device );
void device_wait_idle( const Device* device );

void create_queue( const Device* device, const QueueDesc* desc, Queue** queue );
void destroy_queue( Queue* queue );
void queue_wait_idle( const Queue* queue );
void queue_submit( const Queue* queue, const QueueSubmitDesc* desc );
void immediate_submit( const Queue* queue, const CommandBuffer* cmd );
void queue_present( const Queue* queue, const QueuePresentDesc* desc );

void create_semaphore( const Device* device, Semaphore** semaphore );
void destroy_semaphore( const Device* device, Semaphore* semaphore );
void create_fence( const Device* device, Fence** fence );
void destroy_fence( const Device* device, Fence* fence );
void wait_for_fences( const Device* device, u32 count, Fence** fences );
void reset_fences( const Device* device, u32 count, Fence** fences );

void create_swapchain( const Device*        device,
                       const SwapchainDesc* desc,
                       Swapchain**          swapchain );

void resize_swapchain( const Device* device,
                       Swapchain*    swapchain,
                       u32           width,
                       u32           height );

void destroy_swapchain( const Device* device, Swapchain* swapchain );

void create_command_pool( const Device*          device,
                          const CommandPoolDesc* desc,
                          CommandPool**          command_pool );

void destroy_command_pool( const Device* device, CommandPool* command_pool );

void create_command_buffers( const Device*      device,
                             const CommandPool* command_pool,
                             u32                count,
                             CommandBuffer**    command_buffers );

void free_command_buffers( const Device*      device,
                           const CommandPool* command_pool,
                           u32                count,
                           CommandBuffer**    command_buffers );

void destroy_command_buffers( const Device*      device,
                              const CommandPool* command_pool,
                              u32                count,
                              CommandBuffer**    command_buffers );

void begin_command_buffer( const CommandBuffer* cmd );
void end_command_buffer( const CommandBuffer* cmd );

void acquire_next_image( const Device*    device,
                         const Swapchain* swapchain,
                         const Semaphore* semaphore,
                         const Fence*     fence,
                         u32*             image_index );

void create_render_pass( const Device*         device,
                         const RenderPassDesc* desc,
                         RenderPass**          render_pass );

void update_render_pass( const Device*         device,
                         RenderPass*           render_pass,
                         const RenderPassDesc* desc );

void destroy_render_pass( const Device* device, RenderPass* render_pass );

void create_shader( const Device* device, ShaderDesc* desc, Shader** shader );
void destroy_shader( const Device* device, Shader* shader );

void create_descriptor_set_layout(
    const Device*         device,
    u32                   shader_count,
    Shader**              shaders,
    DescriptorSetLayout** descriptor_set_layout );

void destroy_descriptor_set_layout( const Device*        device,
                                    DescriptorSetLayout* layout );

void create_compute_pipeline( const Device*       device,
                              const PipelineDesc* desc,
                              Pipeline**          pipeline );

void create_graphics_pipeline( const Device*       device,
                               const PipelineDesc* desc,
                               Pipeline**          pipeline );

void destroy_pipeline( const Device* device, Pipeline* pipeline );

void cmd_begin_render_pass( const CommandBuffer*       cmd,
                            const RenderPassBeginDesc* desc );

void cmd_end_render_pass( const CommandBuffer* cmd );

void cmd_barrier( const CommandBuffer* cmd,
                  u32                  memory_barriers_count,
                  const MemoryBarrier* memory_barrier,
                  u32                  buffer_barriers_count,
                  const BufferBarrier* buffer_barriers,
                  u32                  image_barriers_count,
                  const ImageBarrier*  image_barriers );

void cmd_set_scissor( const CommandBuffer* cmd,
                      i32                  x,
                      i32                  y,
                      u32                  width,
                      u32                  height );
void cmd_set_viewport( const CommandBuffer* cmd,
                       f32                  x,
                       f32                  y,
                       f32                  width,
                       f32                  height,
                       f32                  min_depth,
                       f32                  max_depth );

void cmd_bind_pipeline( const CommandBuffer* cmd, const Pipeline* pipeline );
void cmd_draw( const CommandBuffer* cmd,
               u32                  vertex_count,
               u32                  instance_count,
               u32                  first_vertex,
               u32                  first_instance );

void cmd_draw_indexed( const CommandBuffer* cmd,
                       u32                  index_count,
                       u32                  instance_count,
                       u32                  first_index,
                       i32                  vertex_offset,
                       u32                  first_instance );

void cmd_bind_vertex_buffer( const CommandBuffer* cmd,
                             const Buffer*        buffer,
                             const u64            offset );

void cmd_bind_index_buffer_u16( const CommandBuffer* cmd,
                                const Buffer*        buffer,
                                const u64            offset );

void cmd_bind_index_buffer_u32( const CommandBuffer* cmd,
                                const Buffer*        buffer,
                                const u64            offset );

void cmd_copy_buffer( const CommandBuffer* cmd,
                      const Buffer*        src,
                      u64                  src_offset,
                      Buffer*              dst,
                      u64                  dst_offset,
                      u64                  size );

void cmd_copy_buffer_to_image( const CommandBuffer* cmd,
                               const Buffer*        src,
                               u64                  src_offset,
                               Image*               dst );

void cmd_bind_descriptor_set( const CommandBuffer* cmd,
                              u32                  first_set,
                              const DescriptorSet* set,
                              const Pipeline*      pipeline );

void cmd_dispatch( const CommandBuffer* cmd,
                   u32                  group_count_x,
                   u32                  group_count_y,
                   u32                  group_count_z );

void cmd_push_constants( const CommandBuffer* cmd,
                         const Pipeline*      pipeline,
                         u64                  offset,
                         u64                  size,
                         const void*          data );

void cmd_blit_image( const CommandBuffer* cmd,
                     const Image*         src,
                     ResourceState        src_state,
                     Image*               dst,
                     ResourceState        dst_state,
                     Filter               filter );

void cmd_clear_color_image( const CommandBuffer* cmd,
                            Image*               image,
                            Vector4              color );

void create_buffer( const Device*     device,
                    const BufferDesc* desc,
                    Buffer**          buffer );

void destroy_buffer( const Device* device, Buffer* buffer );

void* map_memory( const Device* device, Buffer* buffer );
void  unmap_memory( const Device* device, Buffer* buffer );

void cmd_draw_indexed_indirect( const CommandBuffer* cmd,
                                const Buffer*        buffer,
                                u64                  offset,
                                u32                  draw_count,
                                u32                  stride );

void create_sampler( const Device*      device,
                     const SamplerDesc* desc,
                     Sampler**          sampler );

void destroy_sampler( const Device* device, Sampler* sampler );

void create_image( const Device* device, const ImageDesc* desc, Image** image );
void destroy_image( const Device* device, Image* image );

void create_descriptor_set( const Device*            device,
                            const DescriptorSetDesc* desc,
                            DescriptorSet**          descriptor_set );

void destroy_descriptor_set( const Device* device, DescriptorSet* set );
void update_descriptor_set( const Device*          device,
                            DescriptorSet*         set,
                            u32                    count,
                            const DescriptorWrite* writes );

void create_ui_context( CommandBuffer* cmd,
                        const UiDesc*  desc,
                        UiContext**    ui_context );
void destroy_ui_context( const Device* device, UiContext* context );
void ui_begin_frame();
void ui_end_frame( CommandBuffer* cmd );

} // namespace fluent
