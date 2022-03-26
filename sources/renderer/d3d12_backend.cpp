#ifdef D3D12_BACKEND

#include <tinyimageformat_apis.h>
#include "renderer/renderer_backend.hpp"

#ifdef FLUENT_DEBUG
#define D3D12_ASSERT( x )                                                      \
    do {                                                                       \
        HRESULT err = x;                                                       \
        if ( err )                                                             \
        {                                                                      \
            FT_ERROR( "Detected D3D12 error: {}", err );                       \
            abort();                                                           \
        }                                                                      \
    } while ( 0 )
#else
#define D3D12_ASSERT( x ) x
#endif

namespace fluent
{
static inline DXGI_FORMAT to_dxgi_format( Format format )
{
    return static_cast<DXGI_FORMAT>( TinyImageFormat_ToDXGI_FORMAT(
        static_cast<TinyImageFormat>( format ) ) );
}

static inline D3D12_COMMAND_LIST_TYPE to_d3d12_command_list_type(
    QueueType type )
{
    switch ( type )
    {
    case QueueType::eGraphics: return D3D12_COMMAND_LIST_TYPE_DIRECT;
    case QueueType::eCompute: return D3D12_COMMAND_LIST_TYPE_COMPUTE;
    case QueueType::eTransfer: return D3D12_COMMAND_LIST_TYPE_COPY;
    default: FT_ASSERT( false ); return D3D12_COMMAND_LIST_TYPE( -1 );
    }
}

void create_renderer_backend( const RendererBackendDesc* desc,
                              RendererBackend**          p_backend )
{
    FT_ASSERT( p_backend );

    *p_backend               = new ( std::nothrow ) RendererBackend {};
    RendererBackend* backend = *p_backend;
#ifdef FLUENT_DEBUG
    ID3D12Debug* debug_controller;
    D3D12_ASSERT( D3D12GetDebugInterface( IID_PPV_ARGS( &debug_controller ) ) );
    debug_controller->EnableDebugLayer();
    debug_controller->Release();
    debug_controller = nullptr;
#endif
    D3D12_ASSERT( CreateDXGIFactory1( IID_PPV_ARGS( &backend->p.factory ) ) );
}

void destroy_renderer_backend( RendererBackend* backend )
{
    FT_ASSERT( backend );
    backend->p.factory->Release();
    operator delete( backend, std::nothrow );
}

void create_device( const RendererBackend* backend,
                    const DeviceDesc*      desc,
                    Device**               p_device )
{
    FT_ASSERT( p_device );
    *p_device         = new ( std::nothrow ) Device {};
    Device* device    = *p_device;
    device->p.factory = backend->p.factory;

    // TODO: choose adapter
    D3D12_ASSERT( D3D12CreateDevice( nullptr,
                                     D3D_FEATURE_LEVEL_11_0,
                                     IID_PPV_ARGS( &device->p.device ) ) );
}

void destroy_device( Device* device )
{
    FT_ASSERT( device );
    device->p.device->Release();
    operator delete( device, std::nothrow );
}

void device_wait_idle( const Device* device ) {}

void create_queue( const Device*    device,
                   const QueueDesc* desc,
                   Queue**          p_queue )
{
    FT_ASSERT( p_queue );
    *p_queue     = new ( std::nothrow ) Queue {};
    Queue* queue = *p_queue;

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Type  = to_d3d12_command_list_type( desc->queue_type );
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    D3D12_ASSERT( device->p.device->CreateCommandQueue(
        &queue_desc,
        IID_PPV_ARGS( &queue->p.queue ) ) );
}

void destroy_queue( Queue* queue )
{
    FT_ASSERT( queue );
    queue->p.queue->Release();
    operator delete( queue, std::nothrow );
}

void create_command_pool( const Device*          device,
                          const CommandPoolDesc* desc,
                          CommandPool**          p_command_pool )
{
    FT_ASSERT( device );
    FT_ASSERT( desc->queue );
    FT_ASSERT( p_command_pool );

    *p_command_pool           = new ( std::nothrow ) CommandPool {};
    CommandPool* command_pool = *p_command_pool;
    command_pool->queue       = desc->queue;

    D3D12_ASSERT( device->p.device->CreateCommandAllocator(
        to_d3d12_command_list_type( command_pool->queue->type ),
        IID_PPV_ARGS( &command_pool->p.command_allocator ) ) );
}

void destroy_command_pool( const Device* device, CommandPool* command_pool )
{
    FT_ASSERT( command_pool );
    command_pool->p.command_allocator->Release();
    operator delete( command_pool, std::nothrow );
}

void create_command_buffers( const Device*      device,
                             const CommandPool* command_pool,
                             u32                count,
                             CommandBuffer**    command_buffers )
{
    FT_ASSERT( device );
    FT_ASSERT( command_pool );
    FT_ASSERT( command_buffers );
    FT_ASSERT( count );

    D3D12_COMMAND_LIST_TYPE command_list_type =
        to_d3d12_command_list_type( command_pool->queue->type );

    for ( u32 i = 0; i < count; ++i )
    {
        command_buffers[ i ] = new ( std::nothrow ) CommandBuffer {};
        CommandBuffer* cmd   = command_buffers[ i ];
        cmd->queue           = command_pool->queue;

        D3D12_ASSERT( device->p.device->CreateCommandList(
            0,
            command_list_type,
            command_pool->p.command_allocator,
            nullptr,
            IID_PPV_ARGS( &cmd->p.command_list ) ) );

        cmd->p.command_list->Close();
    }
}

void free_command_buffers( const Device*      device,
                           const CommandPool* command_pool,
                           u32                count,
                           CommandBuffer**    command_buffers )
{
    FT_ASSERT( false );
}

void destroy_command_buffers( const Device*      device,
                              const CommandPool* command_pool,
                              u32                count,
                              CommandBuffer**    command_buffers )
{
    FT_ASSERT( command_buffers );

    for ( u32 i = 0; i < count; ++i )
    {
        command_buffers[ i ]->p.command_list->Release();
        operator delete( command_buffers[ i ], std::nothrow );
    }
}

void create_semaphore( const Device* device, Semaphore** p_semaphore ) {}

void destroy_semaphore( const Device* device, Semaphore* semaphore ) {}

void create_fence( const Device* device, Fence** p_fence )
{
    FT_ASSERT( p_fence );
    *p_fence     = new ( std::nothrow ) Fence {};
    Fence* fence = *p_fence;

    device->p.device->CreateFence( 0,
                                   D3D12_FENCE_FLAG_NONE,
                                   IID_PPV_ARGS( &fence->p.fence ) );
}

void destroy_fence( const Device* device, Fence* fence )
{
    FT_ASSERT( fence );
    fence->p.fence->Release();
    operator delete( fence, std::nothrow );
}

void queue_wait_idle( const Queue* queue ) {}

void queue_submit( const Queue* queue, const QueueSubmitDesc* desc ) {}

void immediate_submit( const Queue* queue, const CommandBuffer* cmd ) {}

void queue_present( const Queue* queue, const QueuePresentDesc* desc ) {}

void wait_for_fences( const Device* device, u32 count, Fence** fences ) {}

void reset_fences( const Device* device, u32 count, Fence** fences ) {}

void create_swapchain( const Device*        device,
                       const SwapchainDesc* desc,
                       Swapchain**          p_swapchain )
{
    FT_ASSERT( p_swapchain );

    *p_swapchain         = new ( std::nothrow ) Swapchain {};
    Swapchain* swapchain = *p_swapchain;
    swapchain->width     = desc->width;
    swapchain->height    = desc->height;
    // TODO:
    swapchain->format          = Format::eB8G8R8A8Unorm;
    swapchain->queue           = desc->queue;
    swapchain->min_image_count = desc->min_image_count;
    swapchain->image_count     = desc->min_image_count;

    DXGI_SWAP_CHAIN_DESC swapchain_desc {};
    swapchain_desc.BufferDesc.Width                   = swapchain->width;
    swapchain_desc.BufferDesc.Height                  = swapchain->height;
    swapchain_desc.BufferDesc.RefreshRate.Numerator   = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_desc.BufferDesc.Format = to_dxgi_format( swapchain->format );
    swapchain_desc.BufferDesc.ScanlineOrdering =
        DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count   = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount        = swapchain->image_count;
    swapchain_desc.OutputWindow       = GetActiveWindow();
    swapchain_desc.Windowed           = true;
    swapchain_desc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchain_desc.Flags              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // Note: Swap chain uses queue to perform flush.
    D3D12_ASSERT(
        device->p.factory->CreateSwapChain( swapchain->queue->p.queue,
                                            &swapchain_desc,
                                            &swapchain->p.swapchain ) );

    swapchain->images = new ( std::nothrow ) Image*[ swapchain->image_count ];
}

void resize_swapchain( const Device* device,
                       Swapchain*    swapchain,
                       u32           width,
                       u32           height )
{
}

void destroy_swapchain( const Device* device, Swapchain* swapchain )
{
    FT_ASSERT( swapchain );
    FT_ASSERT( swapchain->images );
    operator delete[]( swapchain->images, std::nothrow );
    swapchain->p.swapchain->Release();
    operator delete( swapchain, std::nothrow );
}

void begin_command_buffer( const CommandBuffer* cmd ) {}

void end_command_buffer( const CommandBuffer* cmd ) {}

void acquire_next_image( const Device*    device,
                         const Swapchain* swapchain,
                         const Semaphore* semaphore,
                         const Fence*     fence,
                         u32*             image_index )
{
}

void create_framebuffer( const Device*         device,
                         RenderPass*           render_pass,
                         const RenderPassDesc* desc )
{
}

void create_render_pass( const Device*         device,
                         const RenderPassDesc* desc,
                         RenderPass**          p_render_pass )
{
}

void update_render_pass( const Device*         device,
                         RenderPass*           render_pass,
                         const RenderPassDesc* desc )
{
}

void destroy_render_pass( const Device* device, RenderPass* render_pass ) {}

void create_shader( const Device* device, ShaderDesc* desc, Shader** p_shader )
{
}

void destroy_shader( const Device* device, Shader* shader ) {}

void create_descriptor_set_layout(
    const Device*         device,
    u32                   shader_count,
    Shader**              shaders,
    DescriptorSetLayout** p_descriptor_set_layout )
{
}

void destroy_descriptor_set_layout( const Device*        device,
                                    DescriptorSetLayout* layout )
{
}

void create_compute_pipeline( const Device*       device,
                              const PipelineDesc* desc,
                              Pipeline**          p_pipeline )
{
}

void create_graphics_pipeline( const Device*       device,
                               const PipelineDesc* desc,
                               Pipeline**          p_pipeline )
{
}

void destroy_pipeline( const Device* device, Pipeline* pipeline ) {}

void cmd_begin_render_pass( const CommandBuffer*       cmd,
                            const RenderPassBeginDesc* desc )
{
}

void cmd_end_render_pass( const CommandBuffer* cmd ) {}

void cmd_barrier( const CommandBuffer* cmd,
                  u32                  buffer_barriers_count,
                  const BufferBarrier* buffer_barriers,
                  u32                  image_barriers_count,
                  const ImageBarrier*  image_barriers ) {

};

void cmd_set_scissor( const CommandBuffer* cmd,
                      i32                  x,
                      i32                  y,
                      u32                  width,
                      u32                  height )
{
}

void cmd_set_viewport( const CommandBuffer* cmd,
                       f32                  x,
                       f32                  y,
                       f32                  width,
                       f32                  height,
                       f32                  min_depth,
                       f32                  max_depth )
{
}

void cmd_bind_pipeline( const CommandBuffer* cmd, const Pipeline* pipeline ) {}

void cmd_draw( const CommandBuffer* cmd,
               u32                  vertex_count,
               u32                  instance_count,
               u32                  first_vertex,
               u32                  first_instance )
{
}

void cmd_draw_indexed( const CommandBuffer* cmd,
                       u32                  index_count,
                       u32                  instance_count,
                       u32                  first_index,
                       i32                  vertex_offset,
                       u32                  first_instance )
{
}

void cmd_bind_vertex_buffer( const CommandBuffer* cmd,
                             const Buffer*        buffer,
                             const u64            offset )
{
}

void cmd_bind_index_buffer_u16( const CommandBuffer* cmd,
                                const Buffer*        buffer,
                                const u64            offset )
{
}

void cmd_bind_index_buffer_u32( const CommandBuffer* cmd,
                                const Buffer*        buffer,
                                u64                  offset )
{
}

void cmd_copy_buffer( const CommandBuffer* cmd,
                      const Buffer*        src,
                      u64                  src_offset,
                      Buffer*              dst,
                      u64                  dst_offset,
                      u64                  size )
{
}

void cmd_copy_buffer_to_image( const CommandBuffer* cmd,
                               const Buffer*        src,
                               u64                  src_offset,
                               Image*               dst )
{
}

void cmd_dispatch( const CommandBuffer* cmd,
                   u32                  group_count_x,
                   u32                  group_count_y,
                   u32                  group_count_z )
{
}

void cmd_push_constants( const CommandBuffer* cmd,
                         const Pipeline*      pipeline,
                         u64                  offset,
                         u64                  size,
                         const void*          data )
{
}

void cmd_blit_image( const CommandBuffer* cmd,
                     const Image*         src,
                     ResourceState        src_state,
                     Image*               dst,
                     ResourceState        dst_state,
                     Filter               filter )
{
}

void cmd_clear_color_image( const CommandBuffer* cmd,
                            Image*               image,
                            Vector4              color )
{
}

void cmd_draw_indexed_indirect( const CommandBuffer* cmd,
                                const Buffer*        buffer,
                                u64                  offset,
                                u32                  draw_count,
                                u32                  stride )
{
}

void cmd_bind_descriptor_set( const CommandBuffer* cmd,
                              u32                  first_set,
                              const DescriptorSet* set,
                              const Pipeline*      pipeline )
{
}

void create_buffer( const Device*     device,
                    const BufferDesc* desc,
                    Buffer**          p_buffer )
{
}

void destroy_buffer( const Device* device, Buffer* buffer ) {}

void* map_memory( const Device* device, Buffer* buffer ) { return nullptr; }

void unmap_memory( const Device* device, Buffer* buffer ) {}

void create_sampler( const Device*      device,
                     const SamplerDesc* desc,
                     Sampler**          p_sampler )
{
}

void destroy_sampler( const Device* device, Sampler* sampler ) {}

void create_image( const Device*    device,
                   const ImageDesc* desc,
                   Image**          p_image )
{
}

void destroy_image( const Device* device, Image* image ) {}

void create_descriptor_set( const Device*            device,
                            const DescriptorSetDesc* desc,
                            DescriptorSet**          p_descriptor_set )
{
}

void destroy_descriptor_set( const Device* device, DescriptorSet* set ) {}

void update_descriptor_set( const Device*          device,
                            DescriptorSet*         set,
                            u32                    count,
                            const DescriptorWrite* writes )
{
}

void create_ui_context( CommandBuffer* cmd,
                        const UiDesc*  desc,
                        UiContext**    p_context )
{
}

void destroy_ui_context( const Device* device, UiContext* context ) {}

void ui_begin_frame() {}

void ui_end_frame( CommandBuffer* cmd ) {}

} // namespace fluent
#endif
