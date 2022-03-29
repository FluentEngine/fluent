#ifdef D3D12_BACKEND

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_sdl.h>
#include <tinyimageformat_apis.h>
#include "core/window.hpp"
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

static inline D3D12_RESOURCE_STATES to_d3d12_resource_state(
    ResourceState state )
{
    switch ( state )
    {
    case ResourceState::eUndefined: return D3D12_RESOURCE_STATE_COMMON;
    case ResourceState::eGeneral: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    case ResourceState::eColorAttachment:
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case ResourceState::eDepthStencilWrite:
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case ResourceState::eDepthStencilReadOnly:
        return D3D12_RESOURCE_STATE_DEPTH_READ;
    case ResourceState::eShaderReadOnly:
        return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case ResourceState::eTransferSrc: return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case ResourceState::eTransferDst: return D3D12_RESOURCE_STATE_COPY_DEST;
    case ResourceState::ePresent: return D3D12_RESOURCE_STATE_PRESENT;
    default: FT_ASSERT( false ); return D3D12_RESOURCE_STATES( -1 );
    }
}

static inline u32 to_d3d12_sample_count( SampleCount sample_count )
{
    switch ( sample_count )
    {
    case SampleCount::e1: return 1;
    case SampleCount::e2: return 2;
    case SampleCount::e4: return 4;
    case SampleCount::e8: return 8;
    case SampleCount::e16: return 16;
    case SampleCount::e32: return 32;
    case SampleCount::e64: return 64;
    default: FT_ASSERT( false ); return -1;
    }
}

void create_renderer_backend( const RendererBackendDesc* desc,
                              RendererBackend**          p_backend )
{
    FT_ASSERT( p_backend );

    *p_backend               = new ( std::nothrow ) RendererBackend {};
    RendererBackend* backend = *p_backend;
#ifdef FLUENT_DEBUG
    D3D12_ASSERT( D3D12GetDebugInterface(
        IID_PPV_ARGS( &backend->p.debug_controller ) ) );
    backend->p.debug_controller->EnableDebugLayer();
#endif
    D3D12_ASSERT( CreateDXGIFactory1( IID_PPV_ARGS( &backend->p.factory ) ) );
}

void destroy_renderer_backend( RendererBackend* backend )
{
    FT_ASSERT( backend );
    backend->p.factory->Release();
#ifdef FLUENT_DEBUG
    backend->p.debug_controller->Release();
#endif
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

    D3D12_ASSERT( backend->p.factory->EnumAdapters( 0, &device->p.adapter ) );

    // TODO: choose adapter
    D3D12_ASSERT( D3D12CreateDevice( device->p.adapter,
                                     D3D_FEATURE_LEVEL_12_0,
                                     IID_PPV_ARGS( &device->p.device ) ) );

    D3D12MA::ALLOCATOR_DESC allocator_desc = {};
    allocator_desc.Flags                   = {};
    allocator_desc.pDevice                 = device->p.device;
    allocator_desc.pAdapter                = device->p.adapter;

    D3D12MA::CreateAllocator( &allocator_desc, &device->p.allocator );

    D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc;
    rtv_heap_desc.NumDescriptors = 1000;
    rtv_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtv_heap_desc.NodeMask       = 0;
    D3D12_ASSERT( device->p.device->CreateDescriptorHeap(
        &rtv_heap_desc,
        IID_PPV_ARGS( &device->p.rtv_heap ) ) );

    D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc;
    dsv_heap_desc.NumDescriptors = 1000;
    dsv_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsv_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsv_heap_desc.NodeMask       = 0;
    D3D12_ASSERT( device->p.device->CreateDescriptorHeap(
        &dsv_heap_desc,
        IID_PPV_ARGS( &device->p.dsv_heap ) ) );

    device->p.rtv_descriptor_size =
        device->p.device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
    device->p.dsv_descriptor_size =
        device->p.device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV );
}

void destroy_device( Device* device )
{
    FT_ASSERT( device );
    device->p.dsv_heap->Release();
    device->p.rtv_heap->Release();
    device->p.allocator->Release();
    device->p.device->Release();
    device->p.adapter->Release();
    operator delete( device, std::nothrow );
}

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
    D3D12_ASSERT(
        device->p.device->CreateFence( 0,
                                       D3D12_FENCE_FLAG_NONE,
                                       IID_PPV_ARGS( &queue->p.fence ) ) );
}

void destroy_queue( Queue* queue )
{
    FT_ASSERT( queue );
    queue->p.fence->Release();
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
        cmd->p.command_allocator = command_pool->p.command_allocator;
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

void create_semaphore( const Device* device, Semaphore** p_semaphore )
{
    FT_ASSERT( p_semaphore );
    *p_semaphore         = new ( std::nothrow ) Semaphore {};
    Semaphore* semaphore = *p_semaphore;

    device->p.device->CreateFence( 0,
                                   D3D12_FENCE_FLAG_NONE,
                                   IID_PPV_ARGS( &semaphore->p.fence ) );
}

void destroy_semaphore( const Device* device, Semaphore* semaphore )
{
    FT_ASSERT( semaphore );
    semaphore->p.fence->Release();
    operator delete( semaphore, std::nothrow );
}

void create_fence( const Device* device, Fence** p_fence )
{
    FT_ASSERT( p_fence );
    *p_fence     = new ( std::nothrow ) Fence {};
    Fence* fence = *p_fence;

    device->p.device->CreateFence( 0,
                                   D3D12_FENCE_FLAG_NONE,
                                   IID_PPV_ARGS( &fence->p.fence ) );
    fence->p.event_handle = CreateEvent( nullptr, false, false, nullptr );
}

void destroy_fence( const Device* device, Fence* fence )
{
    FT_ASSERT( fence );
    CloseHandle( fence->p.event_handle );
    fence->p.fence->Release();
    operator delete( fence, std::nothrow );
}

void queue_wait_idle( const Queue* queue )
{
    u64 value = queue->p.fence->GetCompletedValue();
    queue->p.queue->Signal( queue->p.fence, value - 1 );
    HANDLE event_handle =
        CreateEventEx( nullptr, nullptr, false, EVENT_ALL_ACCESS );

    // Fire event when GPU hits current fence.
    queue->p.fence->SetEventOnCompletion( value - 1, event_handle );

    // Wait until the GPU hits current fence event is fired.
    WaitForSingleObject( event_handle, INFINITE );
    CloseHandle( event_handle );
}

void queue_submit( const Queue* queue, const QueueSubmitDesc* desc )
{
    std::vector<ID3D12CommandList*> command_lists( desc->command_buffer_count );

    for ( u32 i = 0; i < desc->command_buffer_count; i++ )
    {
        command_lists[ i ] = desc->command_buffers[ i ].p.command_list;
    }

    queue->p.queue->ExecuteCommandLists( desc->command_buffer_count,
                                         command_lists.data() );
}

void immediate_submit( const Queue* queue, const CommandBuffer* cmd ) {}

void queue_present( const Queue* queue, const QueuePresentDesc* desc )
{
    u32 sync_interval = desc->swapchain->vsync ? 1 : 0;
    desc->swapchain->p.swapchain->Present( sync_interval, 0 );
    for ( u32 i = 0; i < desc->wait_semaphore_count; i++ )
    {
        queue->p.queue->Signal( desc->wait_semaphores[ i ].p.fence,
                                desc->wait_semaphores[ i ].p.fence_value );
        desc->wait_semaphores[ i ].p.fence_value++;
    }
}

void wait_for_fences( const Device* device, u32 count, Fence** fences )
{
    for ( u32 i = 0; i < count; i++ )
    {
        // Fire event when GPU hits current fence.
        fences[ i ]->p.fence->SetEventOnCompletion(
            fences[ i ]->p.fence_value,
            fences[ i ]->p.event_handle );

        // Wait until the GPU hits current fence event is fired.
        WaitForSingleObject( fences[ i ]->p.event_handle, INFINITE );
    }
}

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
    swapchain->format          = Format::eR8G8B8A8Unorm;
    swapchain->queue           = desc->queue;
    swapchain->min_image_count = desc->min_image_count;
    swapchain->image_count     = desc->min_image_count;
    swapchain->vsync           = desc->vsync;
    swapchain->p.image_index   = 0;

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

    D3D12_ASSERT( swapchain->p.swapchain->ResizeBuffers(
        swapchain->image_count,
        swapchain->width,
        swapchain->height,
        to_dxgi_format( swapchain->format ),
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH ) );

    swapchain->images = new ( std::nothrow ) Image*[ swapchain->image_count ];

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_heap_handle(
        device->p.rtv_heap->GetCPUDescriptorHandleForHeapStart() );

    for ( u32 i = 0; i < swapchain->image_count; i++ )
    {
        swapchain->images[ i ] = new Image {};
        D3D12_ASSERT( swapchain->p.swapchain->GetBuffer(
            i,
            IID_PPV_ARGS( &swapchain->images[ i ]->p.image ) ) );

        swapchain->images[ i ]->width  = swapchain->width;
        swapchain->images[ i ]->height = swapchain->height;
        swapchain->images[ i ]->depth  = 1;
        swapchain->images[ i ]->format = swapchain->format;

        D3D12_RENDER_TARGET_VIEW_DESC view_desc {};
        view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        // TODO: ...
        view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        device->p.device->CreateRenderTargetView(
            swapchain->images[ i ]->p.image,
            &view_desc,
            rtv_heap_handle );

        swapchain->images[ i ]->p.image_view = rtv_heap_handle;
        rtv_heap_handle.ptr += ( 1 * device->p.rtv_descriptor_size );
    }
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
    for ( u32 i = 0; i < swapchain->image_count; i++ )
    {
        swapchain->images[ i ]->p.image->Release();
        delete swapchain->images[ i ];
    }
    operator delete[]( swapchain->images, std::nothrow );
    swapchain->p.swapchain->Release();
    operator delete( swapchain, std::nothrow );
}

void begin_command_buffer( const CommandBuffer* cmd )
{
    D3D12_ASSERT(
        cmd->p.command_list->Reset( cmd->p.command_allocator, nullptr ) );
}

void end_command_buffer( const CommandBuffer* cmd )
{
    D3D12_ASSERT( cmd->p.command_list->Close() );
}

void acquire_next_image( const Device*    device,
                         const Swapchain* swapchain,
                         const Semaphore* semaphore,
                         const Fence*     fence,
                         u32*             image_index )
{
    *image_index = swapchain->p.image_index;
    swapchain->p.image_index =
        ( swapchain->p.image_index + 1 ) % swapchain->image_count;
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
    FT_ASSERT( p_render_pass );
    *p_render_pass                      = new ( std::nothrow ) RenderPass {};
    RenderPass* render_pass             = *p_render_pass;
    render_pass->color_attachment_count = desc->color_attachment_count;

    for ( u32 i = 0; i < desc->color_attachment_count; i++ )
    {
        render_pass->p.color_attachments[ i ] =
            desc->color_attachments[ i ]->p.image_view;
        render_pass->p.color_formats[ i ] =
            to_dxgi_format( desc->color_attachments[ i ]->format );
    }

    if ( desc->depth_stencil )
    {
        render_pass->has_depth_stencil = true;
        render_pass->p.depth_stencil   = desc->depth_stencil->p.image_view;
        render_pass->p.depth_format =
            to_dxgi_format( desc->depth_stencil->format );
    }
}

void update_render_pass( const Device*         device,
                         RenderPass*           render_pass,
                         const RenderPassDesc* desc )
{
}

void destroy_render_pass( const Device* device, RenderPass* render_pass )
{
    FT_ASSERT( render_pass );
    operator delete( render_pass, std::nothrow );
}

void create_shader( const Device* device, ShaderDesc* desc, Shader** p_shader )
{
    FT_ASSERT( p_shader );
    *p_shader      = new ( std::nothrow ) Shader {};
    Shader* shader = *p_shader;

    shader->stage = desc->stage;

    char* dst = new char[ desc->bytecode_size ];
    char* src = ( char* ) desc->bytecode;
    std::memcpy( dst, src, desc->bytecode_size );
    shader->p.bytecode.BytecodeLength  = desc->bytecode_size;
    shader->p.bytecode.pShaderBytecode = dst;
}

void destroy_shader( const Device* device, Shader* shader )
{
    FT_ASSERT( shader );
    operator delete( ( u8* ) shader->p.bytecode.pShaderBytecode, std::nothrow );
    operator delete( shader, std::nothrow );
}

void create_descriptor_set_layout(
    const Device*         device,
    u32                   shader_count,
    Shader**              shaders,
    DescriptorSetLayout** p_descriptor_set_layout )
{
    FT_ASSERT( p_descriptor_set_layout );
    FT_ASSERT( shader_count );

    *p_descriptor_set_layout = new ( std::nothrow ) DescriptorSetLayout {};
    DescriptorSetLayout* descriptor_set_layout = *p_descriptor_set_layout;

    descriptor_set_layout->shader_count = shader_count;
    descriptor_set_layout->shaders      = shaders;

    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data {};
    feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc {};
    root_signature_desc.Version = feature_data.HighestVersion;
    root_signature_desc.Desc_1_1.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* signature;
    ID3DBlob* error;

    D3D12_ASSERT( D3D12SerializeVersionedRootSignature( &root_signature_desc,
                                                        &signature,
                                                        &error ) );

    D3D12_ASSERT( device->p.device->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS( &descriptor_set_layout->p.root_signature ) ) );

    signature->Release();
    signature = nullptr;
};

void destroy_descriptor_set_layout( const Device*        device,
                                    DescriptorSetLayout* layout )
{
    FT_ASSERT( layout );
    FT_ASSERT( layout->shaders );
    layout->p.root_signature->Release();
    operator delete( layout, std::nothrow );
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
    FT_ASSERT( p_pipeline );
    FT_ASSERT( desc->descriptor_set_layout );
    FT_ASSERT( desc->render_pass );

    *p_pipeline        = new ( std::nothrow ) Pipeline {};
    Pipeline* pipeline = *p_pipeline;

    pipeline->type             = PipelineType::eGraphics;
    pipeline->p.root_signature = desc->descriptor_set_layout->p.root_signature;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_desc {};
    pipeline_desc.pRootSignature =
        desc->descriptor_set_layout->p.root_signature;

    for ( u32 i = 0; i < desc->shader_count; i++ )
    {
        switch ( desc->shaders[ i ]->stage )
        {
        case ShaderStage::eVertex:
        {
            pipeline_desc.VS = desc->shaders[ i ]->p.bytecode;
            break;
        }
        case ShaderStage::eFragment:
        {
            pipeline_desc.PS = desc->shaders[ i ]->p.bytecode;
            break;
        }
        default: break;
        }
    }

    D3D12_RASTERIZER_DESC rasterizer_desc {};
    rasterizer_desc.FillMode              = D3D12_FILL_MODE_SOLID;
    rasterizer_desc.CullMode              = D3D12_CULL_MODE_NONE;
    rasterizer_desc.FrontCounterClockwise = false;
    rasterizer_desc.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer_desc.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer_desc.MultisampleEnable     = false;
    rasterizer_desc.ForcedSampleCount     = 0;
    rasterizer_desc.ConservativeRaster =
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_BLEND_DESC blend_desc {};
    blend_desc.AlphaToCoverageEnable  = false;
    blend_desc.IndependentBlendEnable = false;
    for ( u32 i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++ )
    {
        blend_desc.RenderTarget[ i ] = { false,
                                         false,
                                         D3D12_BLEND_ONE,
                                         D3D12_BLEND_ZERO,
                                         D3D12_BLEND_OP_ADD,
                                         D3D12_BLEND_ONE,
                                         D3D12_BLEND_ZERO,
                                         D3D12_BLEND_OP_ADD,
                                         D3D12_LOGIC_OP_NOOP,
                                         D3D12_COLOR_WRITE_ENABLE_ALL };
    }

    pipeline_desc.NumRenderTargets = desc->render_pass->color_attachment_count;
    for ( u32 i = 0; i < pipeline_desc.NumRenderTargets; i++ )
    {
        // pipeline_desc.RTVFormats[ i ] = desc->render_pass->p.color_formats[ i
        // ];
        // TODO:
        pipeline_desc.RTVFormats[ i ] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    }

    if ( desc->render_pass->has_depth_stencil )
    {
        pipeline_desc.DSVFormat = desc->render_pass->p.depth_format;
    }

    pipeline_desc.RasterizerState = rasterizer_desc;
    pipeline_desc.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipeline_desc.BlendState = blend_desc;
    pipeline_desc.DepthStencilState.DepthEnable =
        desc->depth_state_desc.depth_test;
    pipeline_desc.DepthStencilState.StencilEnable = false;
    pipeline_desc.SampleMask       = std::numeric_limits<u32>::max();
    pipeline_desc.SampleDesc.Count = 1;

    D3D12_ASSERT( device->p.device->CreateGraphicsPipelineState(
        &pipeline_desc,
        IID_PPV_ARGS( &pipeline->p.pipeline ) ) );
}

void destroy_pipeline( const Device* device, Pipeline* pipeline )
{
    FT_ASSERT( pipeline );
    pipeline->p.pipeline->Release();
    operator delete( pipeline, std::nothrow );
}

void cmd_begin_render_pass( const CommandBuffer*       cmd,
                            const RenderPassBeginDesc* desc )
{
    for ( u32 i = 0; i < desc->render_pass->color_attachment_count; i++ )
    {
        cmd->p.command_list->ClearRenderTargetView(
            desc->render_pass->p.color_attachments[ 0 ],
            desc->clear_values[ i ].color,
            0,
            nullptr );
    }

    cmd->p.command_list->OMSetRenderTargets(
        desc->render_pass->color_attachment_count,
        desc->render_pass->p.color_attachments,
        desc->render_pass->has_depth_stencil,
        &desc->render_pass->p.depth_stencil );
}

void cmd_end_render_pass( const CommandBuffer* cmd ) {}

void cmd_barrier( const CommandBuffer* cmd,
                  u32                  memory_barriers_count,
                  const MemoryBarrier* memory_barrier,
                  u32                  buffer_barriers_count,
                  const BufferBarrier* buffer_barriers,
                  u32                  image_barriers_count,
                  const ImageBarrier*  image_barriers )
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers( image_barriers_count );

    u32 barrier_count = 0;

    for ( u32 i = 0; i < image_barriers_count; i++ )
    {
        barriers[ i ]       = {};
        barriers[ i ].Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[ i ].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[ i ].Transition.pResource = image_barriers[ i ].image->p.image;
        barriers[ i ].Transition.StateBefore =
            to_d3d12_resource_state( image_barriers[ i ].old_state );
        barriers[ i ].Transition.StateAfter =
            to_d3d12_resource_state( image_barriers[ i ].new_state );
        barriers[ i ].Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier_count++;
    }

    cmd->p.command_list->ResourceBarrier( barrier_count, barriers.data() );
};

void cmd_set_scissor( const CommandBuffer* cmd,
                      i32                  x,
                      i32                  y,
                      u32                  width,
                      u32                  height )
{
    D3D12_RECT rect;
    rect.left   = x;
    rect.top    = y;
    rect.right  = width;
    rect.bottom = height;

    cmd->p.command_list->RSSetScissorRects( 1, &rect );
}

void cmd_set_viewport( const CommandBuffer* cmd,
                       f32                  x,
                       f32                  y,
                       f32                  width,
                       f32                  height,
                       f32                  min_depth,
                       f32                  max_depth )
{
    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = x;
    viewport.TopLeftY = y;
    viewport.Width    = width;
    viewport.Height   = height;
    viewport.MinDepth = min_depth;
    viewport.MaxDepth = max_depth;

    cmd->p.command_list->RSSetViewports( 1, &viewport );
}

void cmd_bind_pipeline( const CommandBuffer* cmd, const Pipeline* pipeline )
{
    cmd->p.command_list->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    cmd->p.command_list->SetGraphicsRootSignature( pipeline->p.root_signature );
    cmd->p.command_list->SetPipelineState( pipeline->p.pipeline );
}

void cmd_draw( const CommandBuffer* cmd,
               u32                  vertex_count,
               u32                  instance_count,
               u32                  first_vertex,
               u32                  first_instance )
{
    cmd->p.command_list->DrawInstanced( vertex_count,
                                        instance_count,
                                        first_vertex,
                                        first_instance );
}

void cmd_draw_indexed( const CommandBuffer* cmd,
                       u32                  index_count,
                       u32                  instance_count,
                       u32                  first_index,
                       i32                  vertex_offset,
                       u32                  first_instance )
{
    cmd->p.command_list->DrawIndexedInstanced( index_count,
                                               instance_count,
                                               first_index,
                                               vertex_offset,
                                               first_instance );
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
    FT_ASSERT( p_image );

    *p_image     = new ( std::nothrow ) Image {};
    Image* image = *p_image;

    image->width           = desc->width;
    image->height          = desc->height;
    image->depth           = desc->depth;
    image->mip_level_count = desc->mip_levels;
    image->format          = desc->format;
    image->sample_count    = desc->sample_count;

    D3D12_RESOURCE_DESC image_desc = {};
    // TODO:
    image_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    image_desc.Alignment        = 0;
    image_desc.Width            = image->width;
    image_desc.Height           = image->height;
    image_desc.DepthOrArraySize = image->depth;
    image_desc.MipLevels        = image->mip_level_count;
    image_desc.Format           = to_dxgi_format( image->format );
    image_desc.SampleDesc.Count = to_d3d12_sample_count( image->sample_count );
    // TODO:
    image_desc.SampleDesc.Quality = 0;
    image_desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    image_desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12MA::ALLOCATION_DESC alloc_desc = {};
    alloc_desc.HeapType                 = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_ASSERT( device->p.allocator->CreateResource(
        &alloc_desc,
        &image_desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        &image->p.allocation,
        IID_PPV_ARGS( &image->p.image ) ) );

    if ( format_has_depth_aspect( image->format ) ||
         format_has_stencil_aspect( image->format ) )
    {
        D3D12_CPU_DESCRIPTOR_HANDLE dsv_heap_handle(
            device->p.dsv_heap->GetCPUDescriptorHandleForHeapStart() );

        D3D12_DEPTH_STENCIL_VIEW_DESC view_desc {};
        view_desc.Flags              = D3D12_DSV_FLAG_NONE;
        view_desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
        view_desc.Format             = image_desc.Format;
        view_desc.Texture2D.MipSlice = 0;
        device->p.device->CreateDepthStencilView( image->p.image,
                                                  nullptr,
                                                  dsv_heap_handle );
        image->p.image_view = dsv_heap_handle;
        dsv_heap_handle.ptr += ( 1 * device->p.dsv_descriptor_size );
    }
    else
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv_heap_handle(
            device->p.rtv_heap->GetCPUDescriptorHandleForHeapStart() );

        D3D12_RENDER_TARGET_VIEW_DESC view_desc {};
        view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        view_desc.Format        = image_desc.Format;

        device->p.device->CreateRenderTargetView( image->p.image,
                                                  nullptr,
                                                  rtv_heap_handle );

        image->p.image_view = rtv_heap_handle;
        rtv_heap_handle.ptr += ( 1 * device->p.rtv_descriptor_size );
    }
}

void destroy_image( const Device* device, Image* image )
{
    FT_ASSERT( image );
    if ( image->p.allocation )
    {
        image->p.allocation->Release();
    }
    image->p.image->Release();
}

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
    FT_ASSERT( p_context );

    *p_context         = new ( std::nothrow ) UiContext {};
    UiContext* context = *p_context;

    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    ( void ) io;
    if ( desc->docking )
    {
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    if ( desc->viewports )
    {
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NumDescriptors = 1;
    heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    D3D12_ASSERT( desc->device->p.device->CreateDescriptorHeap(
        &heap_desc,
        IID_PPV_ARGS( &context->p.cbv_srv_heap ) ) );

    ImGui_ImplSDL2_InitForD3D( ( SDL_Window* ) desc->window->handle );
    ImGui_ImplDX12_Init(
        desc->device->p.device,
        desc->in_fly_frame_count,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        context->p.cbv_srv_heap,
        context->p.cbv_srv_heap->GetCPUDescriptorHandleForHeapStart(),
        context->p.cbv_srv_heap->GetGPUDescriptorHandleForHeapStart() );
}

void destroy_ui_context( const Device* device, UiContext* context )
{
    FT_ASSERT( context );
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    context->p.cbv_srv_heap->Release();
    ImGui::DestroyContext();
    operator delete( context, std::nothrow );
}

void ui_begin_frame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void ui_end_frame( UiContext* context, CommandBuffer* cmd )
{
    ImGui::Render();
    cmd->p.command_list->SetDescriptorHeaps( 1, &context->p.cbv_srv_heap );
    ImGui_ImplDX12_RenderDrawData( ImGui::GetDrawData(), cmd->p.command_list );

    ImGuiIO& io = ImGui::GetIO();
    if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

} // namespace fluent
#endif
