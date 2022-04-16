#ifdef D3D12_BACKEND

#define INCLUDE_WIN_DEFINES
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_sdl.h>
#include <tinyimageformat_apis.h>
#include <tinyimageformat_query.h>
#include "core/window.hpp"
#include "fs/file_system.hpp"
#include "renderer/renderer_backend.hpp"
#include "renderer/d3d12/d3d12_backend.hpp"

#ifdef FLUENT_DEBUG
#define D3D12_ASSERT( x )                                                      \
    do {                                                                       \
        HRESULT err = x;                                                       \
        if ( err != S_OK )                                                     \
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
static inline DXGI_FORMAT to_dxgi_image_format( Format format )
{
    switch ( format )
    {
    case Format::eR8G8B8A8Srgb: format = Format::eR8G8B8A8Unorm;
    case Format::eB8G8R8A8Srgb: format = Format::eB8G8R8A8Unorm;
    default:
    {
        return static_cast<DXGI_FORMAT>( TinyImageFormat_ToDXGI_FORMAT(
            static_cast<TinyImageFormat>( format ) ) );
        break;
    }
    }
}

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

static inline D3D12_FILL_MODE to_d3d12_fill_mode( PolygonMode mode )
{
    switch ( mode )
    {
    case PolygonMode::eFill: return D3D12_FILL_MODE_SOLID;
    case PolygonMode::eLine: return D3D12_FILL_MODE_WIREFRAME;
    default: FT_ASSERT( false ); return D3D12_FILL_MODE( -1 );
    }
}

static inline D3D12_CULL_MODE to_d3d12_cull_mode( CullMode mode )
{
    switch ( mode )
    {
    case CullMode::eNone: return D3D12_CULL_MODE_NONE;
    case CullMode::eBack: return D3D12_CULL_MODE_BACK;
    case CullMode::eFront: return D3D12_CULL_MODE_FRONT;
    default: FT_ASSERT( false ); return D3D12_CULL_MODE( -1 );
    }
}

static inline b32 to_d3d12_front_face( FrontFace front_face )
{
    if ( front_face == FrontFace::eCounterClockwise )
    {
        return true;
    }
    else
    {
        return false;
    }
}

static inline D3D12_PRIMITIVE_TOPOLOGY_TYPE to_d3d12_primitive_topology_type(
    PrimitiveTopology topology )
{
    switch ( topology )
    {
    case PrimitiveTopology::ePointList:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    case PrimitiveTopology::eLineList:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case PrimitiveTopology::eLineStrip:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case PrimitiveTopology::eTriangleList:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    case PrimitiveTopology::eTriangleStrip:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    case PrimitiveTopology::eTriangleFan:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    default: FT_ASSERT( false ); return D3D12_PRIMITIVE_TOPOLOGY_TYPE( -1 );
    }
}

static inline D3D12_PRIMITIVE_TOPOLOGY to_d3d12_primitive_topology(
    PrimitiveTopology topology )
{
    switch ( topology )
    {
    case PrimitiveTopology::ePointList: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case PrimitiveTopology::eLineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case PrimitiveTopology::eLineStrip: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case PrimitiveTopology::eTriangleList:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case PrimitiveTopology::eTriangleStrip:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case PrimitiveTopology::eTriangleFan:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
    default: FT_ASSERT( false ); return D3D12_PRIMITIVE_TOPOLOGY( -1 );
    }
}

static inline D3D12_HEAP_TYPE to_d3d12_heap_type( MemoryUsage usage )
{
    switch ( usage )
    {
    case MemoryUsage::eGpuOnly: return D3D12_HEAP_TYPE_DEFAULT;
    case MemoryUsage::eCpuOnly: return D3D12_HEAP_TYPE_UPLOAD;
    case MemoryUsage::eCpuToGpu: return D3D12_HEAP_TYPE_UPLOAD;
    case MemoryUsage::eGpuToCpu: return D3D12_HEAP_TYPE_READBACK;
    case MemoryUsage::eCpuCopy: return D3D12_HEAP_TYPE_UPLOAD;
    default: FT_ASSERT( false ); return D3D12_HEAP_TYPE( -1 );
    }
}

static inline D3D12_FILTER_TYPE to_d3d12_filter_type( Filter filter )
{
    switch ( filter )
    {
    case Filter::eNearest: return D3D12_FILTER_TYPE_POINT;
    case Filter::eLinear: return D3D12_FILTER_TYPE_LINEAR;
    default: FT_ASSERT( false ); return D3D12_FILTER_TYPE( -1 );
    }
}

static inline D3D12_FILTER to_d3d12_filter( Filter            min_filter,
                                            Filter            mag_filter,
                                            SamplerMipmapMode mip_map_mode,
                                            b32               anisotropy,
                                            b32 comparison_filter_enabled )
{
    if ( anisotropy )
        return ( comparison_filter_enabled ? D3D12_FILTER_COMPARISON_ANISOTROPIC
                                           : D3D12_FILTER_ANISOTROPIC );

    i32 filter = ( static_cast<int>( min_filter ) << 4 ) |
                 ( static_cast<int>( mag_filter ) << 2 ) |
                 static_cast<int>( mip_map_mode );

    i32 base_filter = comparison_filter_enabled
                          ? D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT
                          : D3D12_FILTER_MIN_MAG_MIP_POINT;

    return ( D3D12_FILTER ) ( base_filter + filter );
}

static inline D3D12_TEXTURE_ADDRESS_MODE to_d3d12_address_mode(
    SamplerAddressMode mode )
{
    switch ( mode )
    {
    case SamplerAddressMode::eRepeat: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case SamplerAddressMode::eMirroredRepeat:
        return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    case SamplerAddressMode::eClampToEdge:
        return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case SamplerAddressMode::eClampToBorder:
        return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    default: FT_ASSERT( false ); return D3D12_TEXTURE_ADDRESS_MODE( -1 );
    }
}

static inline D3D12_COMPARISON_FUNC to_d3d12_comparison_func( CompareOp op )
{
    switch ( op )
    {
    case CompareOp::eNever: return D3D12_COMPARISON_FUNC_NEVER;
    case CompareOp::eLess: return D3D12_COMPARISON_FUNC_LESS;
    case CompareOp::eEqual: return D3D12_COMPARISON_FUNC_EQUAL;
    case CompareOp::eLessOrEqual: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case CompareOp::eGreater: return D3D12_COMPARISON_FUNC_GREATER;
    case CompareOp::eNotEqual: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case CompareOp::eGreaterOrEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case CompareOp::eAlways: return D3D12_COMPARISON_FUNC_ALWAYS;
    default: FT_ASSERT( false ); return D3D12_COMPARISON_FUNC( -1 );
    }
}

static inline D3D12_DESCRIPTOR_RANGE_TYPE to_d3d12_descriptor_range_type(
    DescriptorType type )
{
    switch ( type )
    {
    case DescriptorType::eSampler: return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    case DescriptorType::eStorageBuffer:
    case DescriptorType::eStorageImage:
    case DescriptorType::eStorageTexelBuffer:
        return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    default: return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    }
}

static inline D3D12_SHADER_VISIBILITY to_d3d12_shader_visibility(
    ShaderStage stage )
{
    switch ( stage )
    {
    case ShaderStage::eVertex: return D3D12_SHADER_VISIBILITY_VERTEX;
    case ShaderStage::eTessellationControl: return D3D12_SHADER_VISIBILITY_HULL;
    case ShaderStage::eTessellationEvaluation:
        return D3D12_SHADER_VISIBILITY_DOMAIN;
    case ShaderStage::eGeometry: return D3D12_SHADER_VISIBILITY_GEOMETRY;
    case ShaderStage::eFragment: return D3D12_SHADER_VISIBILITY_PIXEL;
    case ShaderStage::eAllGraphics:
    case ShaderStage::eAll: return D3D12_SHADER_VISIBILITY_ALL;
    default: FT_ASSERT( false ); return D3D12_SHADER_VISIBILITY( -1 );
    }
}

void d3d12_destroy_renderer_backend( RendererBackend* ibackend )
{
    FT_ASSERT( ibackend );

	FT_FROM_HANDLE( backend, ibackend, D3D12RendererBackend );

    backend->factory->Release();
#ifdef FLUENT_DEBUG
    backend->debug_controller->Release();
#endif
    operator delete( backend, std::nothrow );
}

void d3d12_create_device( const RendererBackend* ibackend,
                          const DeviceDesc*      desc,
                          Device**               p )
{
    FT_ASSERT( p );

	FT_FROM_HANDLE( backend, ibackend, D3D12RendererBackend );

    auto device              = new ( std::nothrow ) D3D12Device {};
    device->interface.handle = device;
    *p                       = &device->interface;

    device->factory = backend->factory;

    D3D12_ASSERT( backend->factory->EnumAdapters( 0, &device->adapter ) );

    // TODO: choose adapter
    D3D12_ASSERT( D3D12CreateDevice( device->adapter,
                                     D3D_FEATURE_LEVEL_12_0,
                                     IID_PPV_ARGS( &device->device ) ) );

    D3D12MA::ALLOCATOR_DESC allocator_desc = {};
    allocator_desc.Flags                   = {};
    allocator_desc.pDevice                 = device->device;
    allocator_desc.pAdapter                = device->adapter;

    D3D12MA::CreateAllocator( &allocator_desc, &device->allocator );

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc {};
    heap_desc.NumDescriptors = 1000;
    heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heap_desc.NodeMask       = 0;
    D3D12_ASSERT( device->device->CreateDescriptorHeap(
        &heap_desc,
        IID_PPV_ARGS( &device->rtv_heap ) ) );

    heap_desc.NumDescriptors = 1000;
    heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heap_desc.NodeMask       = 0;
    D3D12_ASSERT( device->device->CreateDescriptorHeap(
        &heap_desc,
        IID_PPV_ARGS( &device->dsv_heap ) ) );

    heap_desc.NumDescriptors = 1000;
    heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heap_desc.NodeMask       = 0;
    D3D12_ASSERT( device->device->CreateDescriptorHeap(
        &heap_desc,
        IID_PPV_ARGS( &device->sampler_heap ) ) );

    heap_desc.NumDescriptors = 1000;
    heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heap_desc.NodeMask       = 0;
    D3D12_ASSERT( device->device->CreateDescriptorHeap(
        &heap_desc,
        IID_PPV_ARGS( &device->cbv_srv_uav_heap ) ) );

    device->rtv_descriptor_size =
        device->device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
    device->dsv_descriptor_size =
        device->device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV );
    device->sampler_descriptor_size =
        device->device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER );
    device->cbv_srv_uav_descriptor_size =
        device->device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
}

void d3d12_destroy_device( Device* idevice )
{
    FT_ASSERT( idevice );

	FT_FROM_HANDLE( device, idevice, D3D12Device );

    device->sampler_heap->Release();
    device->dsv_heap->Release();
    device->rtv_heap->Release();
    device->cbv_srv_uav_heap->Release();
    device->allocator->Release();
    device->device->Release();
    device->adapter->Release();
    operator delete( device, std::nothrow );
}

void d3d12_create_queue( const Device*    idevice,
                         const QueueDesc* desc,
                         Queue**          p )
{
    FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, D3D12Device );

    auto queue              = new ( std::nothrow ) D3D12Queue {};
    queue->interface.handle = queue;
    *p                      = &queue->interface;

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Type  = to_d3d12_command_list_type( desc->queue_type );
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    D3D12_ASSERT(
        device->device->CreateCommandQueue( &queue_desc,
                                            IID_PPV_ARGS( &queue->queue ) ) );
    D3D12_ASSERT(
        device->device->CreateFence( 0,
                                     D3D12_FENCE_FLAG_NONE,
                                     IID_PPV_ARGS( &queue->fence ) ) );
}

void d3d12_destroy_queue( Queue* iqueue )
{
    FT_ASSERT( iqueue );

	FT_FROM_HANDLE( queue, iqueue, D3D12Queue );

    queue->fence->Release();
    queue->queue->Release();
    operator delete( queue, std::nothrow );
}

void d3d12_create_command_pool( const Device*          idevice,
                                const CommandPoolDesc* desc,
                                CommandPool**          p )
{
    FT_ASSERT( idevice );
    FT_ASSERT( desc->queue );
    FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, D3D12Device );

	auto command_pool              = new ( std::nothrow ) D3D12CommandPool {};
    command_pool->interface.handle = command_pool;
    *p                             = &command_pool->interface;

    command_pool->interface.queue = desc->queue;

    D3D12_ASSERT( device->device->CreateCommandAllocator(
        to_d3d12_command_list_type( command_pool->interface.queue->type ),
        IID_PPV_ARGS( &command_pool->command_allocator ) ) );
}

void d3d12_destroy_command_pool( const Device* idevice,
                                 CommandPool*  icommand_pool )
{
    FT_ASSERT( icommand_pool );

	FT_FROM_HANDLE( command_pool, icommand_pool, D3D12CommandPool );

    command_pool->command_allocator->Release();
    operator delete( command_pool, std::nothrow );
}

void d3d12_create_command_buffers( const Device*      idevice,
                                   const CommandPool* icommand_pool,
                                   u32                count,
                                   CommandBuffer**    icommand_buffers )
{
    FT_ASSERT( idevice );
    FT_ASSERT( icommand_pool );
    FT_ASSERT( icommand_buffers );
    FT_ASSERT( count );

	FT_FROM_HANDLE( device, idevice, D3D12Device );
	FT_FROM_HANDLE( command_pool, icommand_pool, D3D12CommandPool );

    D3D12_COMMAND_LIST_TYPE command_list_type =
        to_d3d12_command_list_type( command_pool->interface.queue->type );

    // TODO: FIX ME!
    for ( u32 i = 0; i < count; ++i )
    {
        auto cmd              = new ( std::nothrow ) D3D12CommandBuffer {};
        cmd->interface.handle = cmd;

        cmd->interface.queue = command_pool->interface.queue;

        D3D12_ASSERT( device->device->CreateCommandList(
            0,
            command_list_type,
            command_pool->command_allocator,
            nullptr,
            IID_PPV_ARGS( &cmd->command_list ) ) );
        cmd->command_allocator = command_pool->command_allocator;
        cmd->command_list->Close();

        icommand_buffers[ i ] = &cmd->interface;
    }
}

void d3d12_free_command_buffers( const Device*      idevice,
                                 const CommandPool* icommand_pool,
                                 u32                count,
                                 CommandBuffer**    icommand_buffers )
{
    FT_ASSERT( false );
}

void d3d12_destroy_command_buffers( const Device*      idevice,
                                    const CommandPool* icommand_pool,
                                    u32                count,
                                    CommandBuffer**    icommand_buffers )
{
    FT_ASSERT( icommand_buffers );

    for ( u32 i = 0; i < count; ++i )
    {
		FT_FROM_HANDLE( cmd, icommand_buffers[ i ], D3D12CommandBuffer );
        cmd->command_list->Release();
        operator delete( cmd, std::nothrow );
    }
}

void d3d12_create_semaphore( const Device* idevice, Semaphore** p )
{
    FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, D3D12Device );

	auto semaphore              = new ( std::nothrow ) D3D12Semaphore {};
    semaphore->interface.handle = semaphore;
    *p                          = &semaphore->interface;

    device->device->CreateFence( 0,
                                 D3D12_FENCE_FLAG_NONE,
                                 IID_PPV_ARGS( &semaphore->fence ) );
}

void d3d12_destroy_semaphore( const Device* idevice, Semaphore* isemaphore )
{
    FT_ASSERT( isemaphore );

	FT_FROM_HANDLE( semaphore, isemaphore, D3D12Semaphore );

    semaphore->fence->Release();
    operator delete( semaphore, std::nothrow );
}

void d3d12_create_fence( const Device* idevice, Fence** p )
{
    FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, D3D12Device );

    auto fence              = new ( std::nothrow ) D3D12Fence {};
    fence->interface.handle = fence;
    *p                      = &fence->interface;

    device->device->CreateFence( 0,
                                 D3D12_FENCE_FLAG_NONE,
                                 IID_PPV_ARGS( &fence->fence ) );
    fence->event_handle = CreateEvent( nullptr, false, false, nullptr );
}

void d3d12_destroy_fence( const Device* idevice, Fence* ifence )
{
    FT_ASSERT( ifence );

	FT_FROM_HANDLE( fence, ifence, D3D12Fence );

    CloseHandle( fence->event_handle );
    fence->fence->Release();
    operator delete( fence, std::nothrow );
}

void d3d12_queue_wait_idle( const Queue* iqueue )
{
	FT_FROM_HANDLE( queue, iqueue, D3D12Queue );

	u64 value = queue->fence->GetCompletedValue();
    queue->queue->Signal( queue->fence, value + 1 );
    HANDLE event_handle =
        CreateEventEx( nullptr, nullptr, false, EVENT_ALL_ACCESS );

    // Fire event when GPU hits current fence.
    queue->fence->SetEventOnCompletion( value + 1, event_handle );

    // Wait until the GPU hits current fence event is fired.
    WaitForSingleObject( event_handle, INFINITE );
    CloseHandle( event_handle );
}

void d3d12_queue_submit( const Queue* iqueue, const QueueSubmitDesc* desc )
{
	FT_FROM_HANDLE( queue, iqueue, D3D12Queue );

    std::vector<ID3D12CommandList*> command_lists( desc->command_buffer_count );

    for ( u32 i = 0; i < desc->command_buffer_count; i++ )
    {
		FT_FROM_HANDLE( cmd, desc->command_buffers[ i ], D3D12CommandBuffer );
        command_lists[ i ] = cmd->command_list;
    }

    queue->queue->ExecuteCommandLists( desc->command_buffer_count,
                                       command_lists.data() );
}

void d3d12_immediate_submit( const Queue* iqueue, CommandBuffer* icmd )
{
    QueueSubmitDesc queue_submit_desc {};
    queue_submit_desc.command_buffer_count = 1;
    queue_submit_desc.command_buffers      = &icmd;
    queue_submit( iqueue, &queue_submit_desc );
    queue_wait_idle( iqueue );
}

void d3d12_queue_present( const Queue* iqueue, const QueuePresentDesc* desc )
{
	FT_FROM_HANDLE( queue, iqueue, D3D12Queue );
	FT_FROM_HANDLE( swapchain, desc->swapchain, D3D12Swapchain );

    u32 sync_interval = desc->swapchain->vsync ? 1 : 0;
    swapchain->swapchain->Present( sync_interval, 0 );
    for ( u32 i = 0; i < desc->wait_semaphore_count; i++ )
    {
		FT_FROM_HANDLE( semaphore, desc->wait_semaphores[ i ], D3D12Semaphore );

        queue->queue->Signal( semaphore->fence, semaphore->fence_value );
        semaphore->fence_value++;
    }
}

void d3d12_wait_for_fences( const Device* idevice, u32 count, Fence** ifences )
{
    for ( u32 i = 0; i < count; i++ )
    {
		FT_FROM_HANDLE( fence, ifences[ i ], D3D12Fence );
        // Fire event when GPU hits current fence.
        fence->fence->SetEventOnCompletion( fence->fence_value,
                                            fence->event_handle );

        // Wait until the GPU hits current fence event is fired.
        WaitForSingleObject( fence->event_handle, INFINITE );
    }
}

void d3d12_reset_fences( const Device* idevice, u32 count, Fence** ifences ) {}

void d3d12_create_swapchain( const Device*        idevice,
                             const SwapchainDesc* desc,
                             Swapchain**          p )
{
    FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, D3D12Device );
	FT_FROM_HANDLE( queue, desc->queue, D3D12Queue );

    auto swapchain              = new ( std::nothrow ) D3D12Swapchain {};
    swapchain->interface.handle = swapchain;
    *p                          = &swapchain->interface;

    swapchain->interface.width           = desc->width;
    swapchain->interface.height          = desc->height;
    swapchain->interface.format          = desc->format;
    swapchain->interface.queue           = desc->queue;
    swapchain->interface.min_image_count = desc->min_image_count;
    swapchain->interface.image_count     = desc->min_image_count;
    swapchain->interface.vsync           = desc->vsync;
    swapchain->image_index               = 0;

    DXGI_SWAP_CHAIN_DESC swapchain_desc {};
    swapchain_desc.BufferDesc.Width  = swapchain->interface.width;
    swapchain_desc.BufferDesc.Height = swapchain->interface.height;
    swapchain_desc.BufferDesc.RefreshRate.Numerator   = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_desc.BufferDesc.Format =
        to_dxgi_image_format( swapchain->interface.format );
    swapchain_desc.BufferDesc.ScanlineOrdering =
        DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count   = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount        = swapchain->interface.image_count;
    swapchain_desc.OutputWindow       = GetActiveWindow();
    swapchain_desc.Windowed           = true;
    swapchain_desc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchain_desc.Flags              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    D3D12_ASSERT( device->factory->CreateSwapChain( queue->queue,
                                                    &swapchain_desc,
                                                    &swapchain->swapchain ) );

    D3D12_ASSERT( swapchain->swapchain->ResizeBuffers(
        swapchain->interface.image_count,
        swapchain->interface.width,
        swapchain->interface.height,
        to_dxgi_image_format( swapchain->interface.format ),
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH ) );

    swapchain->interface.images =
        new ( std::nothrow ) Image*[ swapchain->interface.image_count ];

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_heap_handle(
        device->rtv_heap->GetCPUDescriptorHandleForHeapStart() );

    for ( u32 i = 0; i < swapchain->interface.image_count; i++ )
    {
        auto image                       = new ( std::nothrow ) D3D12Image {};
        image->interface.handle          = image;
        swapchain->interface.images[ i ] = &image->interface;

        D3D12_ASSERT(
            swapchain->swapchain->GetBuffer( i,
                                             IID_PPV_ARGS( &image->image ) ) );

        image->interface.width  = swapchain->interface.width;
        image->interface.height = swapchain->interface.height;
        image->interface.depth  = 1;
        image->interface.format = swapchain->interface.format;

        D3D12_RENDER_TARGET_VIEW_DESC view_desc {};
        view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        view_desc.Format        = to_dxgi_format( swapchain->interface.format );

        device->device->CreateRenderTargetView( image->image,
                                                &view_desc,
                                                rtv_heap_handle );

        image->image_view = rtv_heap_handle;
        rtv_heap_handle.ptr += ( 1 * device->rtv_descriptor_size );
    }
}

void d3d12_resize_swapchain( const Device* idevice,
                             Swapchain*    iswapchain,
                             u32           width,
                             u32           height )
{
}

void d3d12_destroy_swapchain( const Device* idevice, Swapchain* iswapchain )
{
    FT_ASSERT( iswapchain );

	FT_FROM_HANDLE( swapchain, iswapchain, D3D12Swapchain );

    for ( u32 i = 0; i < swapchain->interface.image_count; i++ )
    {
		FT_FROM_HANDLE( image, swapchain->interface.images[ i ], D3D12Image );
        image->image->Release();
        operator delete( image, std::nothrow );
    }
    operator delete[]( swapchain->interface.images, std::nothrow );
    swapchain->swapchain->Release();
    operator delete( swapchain, std::nothrow );
}

void d3d12_begin_command_buffer( const CommandBuffer* icmd )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );
    D3D12_ASSERT( cmd->command_list->Reset( cmd->command_allocator, nullptr ) );
}

void d3d12_end_command_buffer( const CommandBuffer* icmd )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );
    D3D12_ASSERT( cmd->command_list->Close() );
}

void d3d12_acquire_next_image( const Device*    idevice,
                               const Swapchain* iswapchain,
                               const Semaphore* isemaphore,
                               const Fence*     ifence,
                               u32*             image_index )
{
	FT_FROM_HANDLE( swapchain, iswapchain, D3D12Swapchain );

	*image_index = swapchain->image_index;
    swapchain->image_index =
        ( swapchain->image_index + 1 ) % swapchain->interface.image_count;
}

void d3d12_create_render_pass( const Device*         idevice,
                               const RenderPassDesc* desc,
                               RenderPass**          p )
{
    FT_ASSERT( p );
    auto render_pass              = new ( std::nothrow ) D3D12RenderPass {};
    render_pass->interface.handle = render_pass;
    *p                            = &render_pass->interface;

    render_pass->interface.color_attachment_count =
        desc->color_attachment_count;

    for ( u32 i = 0; i < desc->color_attachment_count; i++ )
    {
		FT_FROM_HANDLE( image, desc->color_attachments[ i ], D3D12Image );

        render_pass->color_attachments[ i ] = image->image_view;
        render_pass->color_formats[ i ] =
            to_dxgi_format( desc->color_attachments[ i ]->format );
    }

    if ( desc->depth_stencil )
    {
		FT_FROM_HANDLE( image, desc->depth_stencil, D3D12Image );

        render_pass->interface.has_depth_stencil = true;
        render_pass->depth_stencil               = image->image_view;
        render_pass->depth_format =
            to_dxgi_format( desc->depth_stencil->format );
    }
}

void d3d12_update_render_pass( const Device*         idevice,
                               RenderPass*           irender_pass,
                               const RenderPassDesc* desc )
{
}

void d3d12_destroy_render_pass( const Device* idevice,
                                RenderPass*   irender_pass )
{
    FT_ASSERT( irender_pass );

	FT_FROM_HANDLE( render_pass, irender_pass, D3D12RenderPass );

    operator delete( render_pass, std::nothrow );
}

void d3d12_create_shader( const Device* idevice, ShaderDesc* desc, Shader** p )
{
    FT_ASSERT( p );

    auto shader              = new ( std::nothrow ) D3D12Shader {};
    shader->interface.handle = shader;
    *p                       = &shader->interface;

    shader->interface.stage = desc->stage;

    char* dst = new char[ desc->bytecode_size ];
    char* src = ( char* ) desc->bytecode;
    std::memcpy( dst, src, desc->bytecode_size );
    shader->bytecode.BytecodeLength  = desc->bytecode_size;
    shader->bytecode.pShaderBytecode = dst;

    shader->interface.reflect_data =
        dxil_reflect( shader->bytecode.BytecodeLength,
                      shader->bytecode.pShaderBytecode );
}

void d3d12_destroy_shader( const Device* idevice, Shader* ishader )
{
    FT_ASSERT( ishader );

	FT_FROM_HANDLE( shader, ishader, D3D12Shader );

    operator delete( ( u8* ) shader->bytecode.pShaderBytecode, std::nothrow );
    operator delete( shader, std::nothrow );
}

void d3d12_create_descriptor_set_layout( const Device*         idevice,
                                         u32                   shader_count,
                                         Shader**              ishaders,
                                         DescriptorSetLayout** p )
{
    FT_ASSERT( p );
    FT_ASSERT( shader_count );

	FT_FROM_HANDLE( device, idevice, D3D12Device );

    auto descriptor_set_layout =
        new ( std::nothrow ) D3D12DescriptorSetLayout {};
    descriptor_set_layout->interface.handle = descriptor_set_layout;
    *p                                      = &descriptor_set_layout->interface;

    descriptor_set_layout->interface.shader_count = shader_count;
    descriptor_set_layout->interface.shaders      = ishaders;

    std::vector<D3D12_ROOT_PARAMETER1>   tables;
    std::vector<D3D12_DESCRIPTOR_RANGE1> cbv_ranges;
    std::vector<D3D12_DESCRIPTOR_RANGE1> srv_uav_ranges;
    std::vector<D3D12_DESCRIPTOR_RANGE1> sampler_ranges;

    for ( u32 s = 0; s < shader_count; ++s )
    {
        u32 cbv_range_count     = cbv_ranges.size();
        u32 srv_uav_range_count = srv_uav_ranges.size();
        u32 sampler_range_count = sampler_ranges.size();

        for ( u32 b = 0; b < ishaders[ s ]->reflect_data.binding_count; ++b )
        {
            const auto& binding = ishaders[ s ]->reflect_data.bindings[ b ];
            D3D12_DESCRIPTOR_RANGE1 range = {};
            range.BaseShaderRegister      = binding.binding;
            range.RangeType =
                to_d3d12_descriptor_range_type( binding.descriptor_type );
            range.NumDescriptors                    = binding.descriptor_count;
            range.RegisterSpace                     = binding.set;
            range.OffsetInDescriptorsFromTableStart = 0;

            switch ( range.RangeType )
            {
            case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
                cbv_ranges.emplace_back( std::move( range ) );
                break;
            case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
            case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                srv_uav_ranges.emplace_back( std::move( range ) );
                break;
            case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
                sampler_ranges.emplace_back( std::move( range ) );
                break;
            default: break;
            }
        }

        if ( cbv_ranges.size() - cbv_range_count > 0 )
        {
            D3D12_ROOT_PARAMETER1& table = tables.emplace_back();
            table                        = {};
            table.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            table.ShaderVisibility =
                to_d3d12_shader_visibility( ishaders[ s ]->stage );
            table.DescriptorTable.NumDescriptorRanges =
                cbv_ranges.size() - cbv_range_count;
            table.DescriptorTable.pDescriptorRanges =
                cbv_ranges.data() + cbv_range_count;
        }

        if ( srv_uav_ranges.size() - srv_uav_range_count > 0 )
        {
            D3D12_ROOT_PARAMETER1& table = tables.emplace_back();
            table                        = {};
            table.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            table.ShaderVisibility =
                to_d3d12_shader_visibility( ishaders[ s ]->stage );
            table.DescriptorTable.NumDescriptorRanges =
                srv_uav_ranges.size() - srv_uav_range_count;
            table.DescriptorTable.pDescriptorRanges =
                srv_uav_ranges.data() + srv_uav_range_count;
        }

        if ( sampler_ranges.size() - sampler_range_count > 0 )
        {
            D3D12_ROOT_PARAMETER1& table = tables.emplace_back();
            table                        = {};
            table.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            table.ShaderVisibility =
                to_d3d12_shader_visibility( ishaders[ s ]->stage );
            table.DescriptorTable.NumDescriptorRanges =
                sampler_ranges.size() - sampler_range_count;
            table.DescriptorTable.pDescriptorRanges =
                sampler_ranges.data() + sampler_range_count;
        }
    }

    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data {};
    feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc {};
    root_signature_desc.Version = feature_data.HighestVersion;
    root_signature_desc.Desc_1_1.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    root_signature_desc.Desc_1_1.NumParameters = tables.size();
    root_signature_desc.Desc_1_1.pParameters   = tables.data();

    ID3DBlob* signature;
    ID3DBlob* error;

    D3D12_ASSERT( D3D12SerializeVersionedRootSignature( &root_signature_desc,
                                                        &signature,
                                                        &error ) );

    D3D12_ASSERT( device->device->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS( &descriptor_set_layout->root_signature ) ) );

    signature->Release();
    signature = nullptr;
};

void d3d12_destroy_descriptor_set_layout( const Device*        idevice,
                                          DescriptorSetLayout* ilayout )
{
    FT_ASSERT( ilayout );

	FT_FROM_HANDLE( layout, ilayout, D3D12DescriptorSetLayout );

    layout->root_signature->Release();
    operator delete( layout, std::nothrow );
}

void d3d12_create_compute_pipeline( const Device*       idevice,
                                    const PipelineDesc* desc,
                                    Pipeline**          p )
{
}

void d3d12_create_graphics_pipeline( const Device*       idevice,
                                     const PipelineDesc* desc,
                                     Pipeline**          p )
{
    FT_ASSERT( p );
    FT_ASSERT( desc->descriptor_set_layout );
    FT_ASSERT( desc->render_pass );

    auto pipeline              = new ( std::nothrow ) D3D12Pipeline {};
    pipeline->interface.handle = pipeline;
    *p                         = &pipeline->interface;

	FT_FROM_HANDLE( device, idevice, D3D12Device );
	FT_FROM_HANDLE( dsl,
					desc->descriptor_set_layout,
					D3D12DescriptorSetLayout );
	FT_FROM_HANDLE( render_pass, desc->render_pass, D3D12RenderPass );

    pipeline->interface.type = PipelineType::eGraphics;
    pipeline->root_signature = dsl->root_signature;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_desc {};
    pipeline_desc.pRootSignature = dsl->root_signature;

    for ( u32 i = 0; i < desc->shader_count; i++ )
    {
		FT_FROM_HANDLE( shader, desc->shaders[ i ], D3D12Shader );

        switch ( desc->shaders[ i ]->stage )
        {
        case ShaderStage::eVertex:
        {
            pipeline_desc.VS = shader->bytecode;
            break;
        }
        case ShaderStage::eFragment:
        {
            pipeline_desc.PS = shader->bytecode;
            break;
        }
        default: break;
        }
    }

    const VertexLayout& vertex_layout = desc->vertex_layout;

    D3D12_INPUT_LAYOUT_DESC  input_layout {};
    D3D12_INPUT_ELEMENT_DESC input_elements[ MAX_VERTEX_ATTRIBUTE_COUNT ];
    u32                      input_element_count = 0;

    for ( u32 i = 0; i < vertex_layout.attribute_desc_count; i++ )
    {
        const auto& attribute     = vertex_layout.attribute_descs[ i ];
        auto&       input_element = input_elements[ input_element_count ];

        input_element.Format            = to_dxgi_format( attribute.format );
        input_element.InputSlot         = attribute.binding;
        input_element.AlignedByteOffset = attribute.offset;
        input_element.SemanticIndex     = 0;
        input_element.SemanticName      = "TODO";

        if ( vertex_layout.binding_descs[ attribute.binding ].input_rate ==
             VertexInputRate::eInstance )
        {
            input_element.InputSlotClass =
                D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
            input_element.InstanceDataStepRate = 1;
        }
        else
        {
            input_element.InputSlotClass =
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            input_element.InstanceDataStepRate = 0;
        }
        input_element_count++;
    }

    input_layout.NumElements        = input_element_count;
    input_layout.pInputElementDescs = input_elements;

    D3D12_RASTERIZER_DESC rasterizer_desc {};
    rasterizer_desc.FillMode =
        to_d3d12_fill_mode( desc->rasterizer_desc.polygon_mode );
    rasterizer_desc.CullMode =
        to_d3d12_cull_mode( desc->rasterizer_desc.cull_mode );
    rasterizer_desc.FrontCounterClockwise =
        to_d3d12_front_face( desc->rasterizer_desc.front_face );
    rasterizer_desc.DepthBias         = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer_desc.DepthBiasClamp    = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer_desc.MultisampleEnable = false;
    rasterizer_desc.ForcedSampleCount = 0;
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
        pipeline_desc.RTVFormats[ i ] = render_pass->color_formats[ i ];
    }

    if ( desc->render_pass->has_depth_stencil )
    {
        pipeline_desc.DSVFormat = render_pass->depth_format;
    }

    pipeline_desc.InputLayout     = input_layout;
    pipeline_desc.RasterizerState = rasterizer_desc;
    pipeline_desc.PrimitiveTopologyType =
        to_d3d12_primitive_topology_type( desc->topology );
    pipeline->topology       = to_d3d12_primitive_topology( desc->topology );
    pipeline_desc.BlendState = blend_desc;
    pipeline_desc.DepthStencilState.DepthEnable =
        desc->depth_state_desc.depth_test;
    pipeline_desc.DepthStencilState.StencilEnable = false;
    pipeline_desc.SampleMask       = std::numeric_limits<u32>::max();
    pipeline_desc.SampleDesc.Count = 1;

    D3D12_ASSERT( device->device->CreateGraphicsPipelineState(
        &pipeline_desc,
        IID_PPV_ARGS( &pipeline->pipeline ) ) );
}

void d3d12_destroy_pipeline( const Device* idevice, Pipeline* ipipeline )
{
    FT_ASSERT( ipipeline );

	FT_FROM_HANDLE( pipeline, ipipeline, D3D12Pipeline );

    pipeline->pipeline->Release();
    operator delete( pipeline, std::nothrow );
}

void d3d12_cmd_begin_render_pass( const CommandBuffer*       icmd,
                                  const RenderPassBeginDesc* desc )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );
	FT_FROM_HANDLE( render_pass, desc->render_pass, D3D12RenderPass );

    for ( u32 i = 0; i < desc->render_pass->color_attachment_count; i++ )
    {
        cmd->command_list->ClearRenderTargetView(
            render_pass->color_attachments[ 0 ],
            desc->clear_values[ i ].color,
            0,
            nullptr );
    }

    cmd->command_list->OMSetRenderTargets(
        desc->render_pass->color_attachment_count,
        render_pass->color_attachments,
        desc->render_pass->has_depth_stencil,
        &render_pass->depth_stencil );
}

void d3d12_cmd_end_render_pass( const CommandBuffer* icmd ) {}

void d3d12_cmd_barrier( const CommandBuffer* icmd,
                        u32                  memory_barriers_count,
                        const MemoryBarrier* memory_barrier,
                        u32                  buffer_barriers_count,
                        const BufferBarrier* buffer_barriers,
                        u32                  image_barriers_count,
                        const ImageBarrier*  image_barriers )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );

    std::vector<D3D12_RESOURCE_BARRIER> barriers( image_barriers_count );

    u32 barrier_count = 0;

    for ( u32 i = 0; i < image_barriers_count; i++ )
    {
		FT_FROM_HANDLE( image, image_barriers[ i ].image, D3D12Image );

        barriers[ i ]       = {};
        barriers[ i ].Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[ i ].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[ i ].Transition.pResource = image->image;
        barriers[ i ].Transition.StateBefore =
            to_d3d12_resource_state( image_barriers[ i ].old_state );
        barriers[ i ].Transition.StateAfter =
            to_d3d12_resource_state( image_barriers[ i ].new_state );
        barriers[ i ].Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier_count++;
    }

    cmd->command_list->ResourceBarrier( barrier_count, barriers.data() );
};

void d3d12_cmd_set_scissor( const CommandBuffer* icmd,
                            i32                  x,
                            i32                  y,
                            u32                  width,
                            u32                  height )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );

    D3D12_RECT rect;
    rect.left   = x;
    rect.top    = y;
    rect.right  = width;
    rect.bottom = height;

    cmd->command_list->RSSetScissorRects( 1, &rect );
}

void d3d12_cmd_set_viewport( const CommandBuffer* icmd,
                             f32                  x,
                             f32                  y,
                             f32                  width,
                             f32                  height,
                             f32                  min_depth,
                             f32                  max_depth )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );

    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = x;
    viewport.TopLeftY = y;
    viewport.Width    = width;
    viewport.Height   = height;
    viewport.MinDepth = min_depth;
    viewport.MaxDepth = max_depth;

    cmd->command_list->RSSetViewports( 1, &viewport );
}

void d3d12_cmd_bind_pipeline( const CommandBuffer* icmd,
                              const Pipeline*      ipipeline )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );
	FT_FROM_HANDLE( pipeline, ipipeline, D3D12Pipeline );

    cmd->command_list->IASetPrimitiveTopology( pipeline->topology );
    cmd->command_list->SetGraphicsRootSignature( pipeline->root_signature );
    cmd->command_list->SetPipelineState( pipeline->pipeline );
}

void d3d12_cmd_draw( const CommandBuffer* icmd,
                     u32                  vertex_count,
                     u32                  instance_count,
                     u32                  first_vertex,
                     u32                  first_instance )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );

    cmd->command_list->DrawInstanced( vertex_count,
									  instance_count,
									  first_vertex,
									  first_instance );
}

void d3d12_cmd_draw_indexed( const CommandBuffer* icmd,
                             u32                  index_count,
                             u32                  instance_count,
                             u32                  first_index,
                             i32                  vertex_offset,
                             u32                  first_instance )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );

    cmd->command_list->DrawIndexedInstanced( index_count,
											 instance_count,
											 first_index,
											 vertex_offset,
											 first_instance );
}

void d3d12_cmd_bind_vertex_buffer( const CommandBuffer* icmd,
                                   const Buffer*        ibuffer,
                                   const u64            offset )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );
	FT_FROM_HANDLE( buffer, ibuffer, D3D12Buffer );

    D3D12_VERTEX_BUFFER_VIEW buffer_view {};
    buffer_view.BufferLocation = buffer->buffer->GetGPUVirtualAddress();
    buffer_view.SizeInBytes    = buffer->interface.size;
    buffer_view.StrideInBytes  = 4 * sizeof( float );
    cmd->command_list->IASetVertexBuffers( 0, 1, &buffer_view );
}

void d3d12_cmd_bind_index_buffer_u16( const CommandBuffer* cmd,
                                      const Buffer*        buffer,
                                      const u64            offset )
{
}

void d3d12_cmd_bind_index_buffer_u32( const CommandBuffer* cmd,
                                      const Buffer*        buffer,
                                      u64                  offset )
{
}

void d3d12_cmd_copy_buffer( const CommandBuffer* icmd,
                            const Buffer*        isrc,
                            u64                  src_offset,
                            Buffer*              idst,
                            u64                  dst_offset,
                            u64                  size )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );
	FT_FROM_HANDLE( src, isrc, D3D12Buffer );
	FT_FROM_HANDLE( dst, idst, D3D12Buffer );

	cmd->command_list->CopyBufferRegion( dst->buffer,
										 dst_offset,
										 src->buffer,
										 src_offset,
										 size );
}

void d3d12_cmd_copy_buffer_to_image( const CommandBuffer* icmd,
                                     const Buffer*        isrc,
                                     u64                  src_offset,
                                     Image*               idst )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );
	FT_FROM_HANDLE( src, isrc, D3D12Buffer );
	FT_FROM_HANDLE( dst, idst, D3D12Image );

    D3D12_TEXTURE_COPY_LOCATION dst_copy_location {};
    dst_copy_location.SubresourceIndex = 0;
    dst_copy_location.Type      = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst_copy_location.pResource = dst->image;

    u32 texel_size = TinyImageFormat_BitSizeOfBlock(
                         ( TinyImageFormat ) dst->interface.format ) /
                     8;

    D3D12_TEXTURE_COPY_LOCATION src_copy_location {};
    src_copy_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src_copy_location.PlacedFootprint.Footprint.Format =
        to_dxgi_image_format( dst->interface.format );
    src_copy_location.PlacedFootprint.Footprint.Width  = dst->interface.width;
    src_copy_location.PlacedFootprint.Footprint.Height = dst->interface.height;
    src_copy_location.PlacedFootprint.Footprint.Depth  = dst->interface.depth;
    src_copy_location.PlacedFootprint.Footprint.RowPitch =
        texel_size * dst->interface.width;
    src_copy_location.PlacedFootprint.Offset = 0;
	src_copy_location.pResource              = src->buffer;

    cmd->command_list->CopyTextureRegion( &dst_copy_location,
										  0,
										  0,
										  0,
										  &src_copy_location,
										  nullptr );
}

void d3d12_cmd_dispatch( const CommandBuffer* cmd,
                         u32                  group_count_x,
                         u32                  group_count_y,
                         u32                  group_count_z )
{
}

void d3d12_cmd_push_constants( const CommandBuffer* cmd,
                               const Pipeline*      pipeline,
                               u64                  offset,
                               u64                  size,
                               const void*          data )
{
}

void d3d12_cmd_blit_image( const CommandBuffer* cmd,
                           const Image*         src,
                           ResourceState        src_state,
                           Image*               dst,
                           ResourceState        dst_state,
                           Filter               filter )
{
}

void d3d12_cmd_clear_color_image( const CommandBuffer* cmd,
                                  Image*               image,
                                  Vector4              color )
{
}

void d3d12_cmd_draw_indexed_indirect( const CommandBuffer* cmd,
                                      const Buffer*        buffer,
                                      u64                  offset,
                                      u32                  draw_count,
                                      u32                  stride )
{
}

void d3d12_cmd_bind_descriptor_set( const CommandBuffer* cmd,
                                    u32                  first_set,
                                    const DescriptorSet* set,
                                    const Pipeline*      pipeline )
{
}

void d3d12_create_buffer( const Device*     idevice,
                          const BufferDesc* desc,
                          Buffer**          p )
{
    FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, D3D12Device );

    auto buffer              = new ( std::nothrow ) D3D12Buffer {};
    buffer->interface.handle = buffer;
    *p                       = &buffer->interface;

    buffer->interface.size            = desc->size;
    buffer->interface.descriptor_type = desc->descriptor_type;
    buffer->interface.memory_usage    = desc->memory_usage;

    D3D12_RESOURCE_DESC buffer_desc {};
    buffer_desc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer_desc.Alignment        = 65536ul;
    buffer_desc.Width            = buffer->interface.size;
    buffer_desc.Height           = 1u;
    buffer_desc.DepthOrArraySize = 1;
    buffer_desc.MipLevels        = 1;
    buffer_desc.Format           = DXGI_FORMAT_UNKNOWN;
    buffer_desc.SampleDesc       = { 1u, 0u };
    buffer_desc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    buffer_desc.Flags            = D3D12_RESOURCE_FLAG_NONE;

    D3D12MA::ALLOCATION_DESC alloc_desc = {};
    alloc_desc.HeapType = to_d3d12_heap_type( desc->memory_usage );

    D3D12_ASSERT(
        device->allocator->CreateResource( &alloc_desc,
                                           &buffer_desc,
                                           D3D12_RESOURCE_STATE_GENERIC_READ,
                                           nullptr,
                                           &buffer->allocation,
                                           IID_PPV_ARGS( &buffer->buffer ) ) );
}

void d3d12_destroy_buffer( const Device* idevice, Buffer* ibuffer )
{
    FT_ASSERT( ibuffer );

	FT_FROM_HANDLE( buffer, ibuffer, D3D12Buffer );
    buffer->allocation->Release();
    buffer->buffer->Release();
    operator delete( buffer, std::nothrow );
}

void* d3d12_map_memory( const Device* idevice, Buffer* ibuffer )
{
    FT_ASSERT( ibuffer != nullptr );
    FT_ASSERT( ibuffer->mapped_memory == nullptr );

	FT_FROM_HANDLE( buffer, ibuffer, D3D12Buffer );

    D3D12_ASSERT(
        buffer->buffer->Map( 0, nullptr, &buffer->interface.mapped_memory ) );
    return buffer->interface.mapped_memory;
}

void d3d12_unmap_memory( const Device* idevice, Buffer* ibuffer )
{
    FT_ASSERT( ibuffer );
    FT_ASSERT( ibuffer->mapped_memory );

	FT_FROM_HANDLE( buffer, ibuffer, D3D12Buffer );

    buffer->buffer->Unmap( 0, nullptr );
    buffer->interface.mapped_memory = nullptr;
}

void d3d12_create_sampler( const Device*      idevice,
                           const SamplerDesc* desc,
                           Sampler**          p )
{
    FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, D3D12Device );

    auto sampler = new ( std::nothrow ) D3D12Sampler {};

    D3D12_SAMPLER_DESC sampler_desc {};
    sampler_desc.Filter         = to_d3d12_filter( desc->min_filter,
                                           desc->mag_filter,
                                           desc->mipmap_mode,
                                           desc->anisotropy_enable,
                                           desc->compare_enable );
    sampler_desc.AddressU       = to_d3d12_address_mode( desc->address_mode_u );
    sampler_desc.AddressV       = to_d3d12_address_mode( desc->address_mode_v );
    sampler_desc.AddressW       = to_d3d12_address_mode( desc->address_mode_w );
    sampler_desc.MipLODBias     = desc->mip_lod_bias;
    sampler_desc.MaxAnisotropy  = desc->max_anisotropy;
    sampler_desc.ComparisonFunc = to_d3d12_comparison_func( desc->compare_op );
    sampler_desc.MinLOD         = desc->min_lod;
    sampler_desc.MaxLOD         = desc->max_lod;

    D3D12_CPU_DESCRIPTOR_HANDLE sampler_heap_handle(
        device->sampler_heap->GetCPUDescriptorHandleForHeapStart() );
    device->device->CreateSampler( &sampler_desc, sampler_heap_handle );
    sampler->handle = sampler_heap_handle;
    sampler_heap_handle.ptr += ( 1 * device->sampler_descriptor_size );
}

void d3d12_destroy_sampler( const Device* device, Sampler* isampler )
{
    FT_ASSERT( isampler );

	FT_FROM_HANDLE( sampler, isampler, D3D12Sampler );

	operator delete( sampler, std::nothrow );
}

void d3d12_create_image( const Device*    idevice,
                         const ImageDesc* desc,
                         Image**          p )
{
    FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, D3D12Device );

    auto image              = new ( std::nothrow ) D3D12Image {};
    image->interface.handle = image;
    *p                      = &image->interface;

    image->interface.width           = desc->width;
    image->interface.height          = desc->height;
    image->interface.depth           = desc->depth;
    image->interface.mip_level_count = desc->mip_levels;
    image->interface.format          = desc->format;
    image->interface.sample_count    = desc->sample_count;

    D3D12_RESOURCE_DESC image_desc = {};
    // TODO:
    image_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    image_desc.Alignment        = 0;
    image_desc.Width            = image->interface.width;
    image_desc.Height           = image->interface.height;
    image_desc.DepthOrArraySize = image->interface.depth;
    image_desc.MipLevels        = image->interface.mip_level_count;
    image_desc.Format = to_dxgi_image_format( image->interface.format );
    image_desc.SampleDesc.Count =
        to_d3d12_sample_count( image->interface.sample_count );
    // TODO:
    image_desc.SampleDesc.Quality = 0;
    image_desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    if ( format_has_depth_aspect( desc->format ) ||
         format_has_stencil_aspect( desc->format ) )
    {
        image_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }

    D3D12MA::ALLOCATION_DESC alloc_desc = {};
    alloc_desc.HeapType                 = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_ASSERT(
        device->allocator->CreateResource( &alloc_desc,
                                           &image_desc,
                                           D3D12_RESOURCE_STATE_COMMON,
                                           nullptr,
                                           &image->allocation,
                                           IID_PPV_ARGS( &image->image ) ) );

    if ( format_has_depth_aspect( image->interface.format ) ||
         format_has_stencil_aspect( image->interface.format ) )
    {
        D3D12_CPU_DESCRIPTOR_HANDLE dsv_heap_handle(
            device->dsv_heap->GetCPUDescriptorHandleForHeapStart() );

        D3D12_DEPTH_STENCIL_VIEW_DESC view_desc {};
        view_desc.Flags              = D3D12_DSV_FLAG_NONE;
        view_desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
        view_desc.Format             = image_desc.Format;
        view_desc.Texture2D.MipSlice = 0;
        device->device->CreateDepthStencilView( image->image,
                                                nullptr,
                                                dsv_heap_handle );
        image->image_view = dsv_heap_handle;
        dsv_heap_handle.ptr += ( 1 * device->dsv_descriptor_size );
    }
    else if ( desc->descriptor_type == DescriptorType::eColorAttachment )
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv_heap_handle(
            device->rtv_heap->GetCPUDescriptorHandleForHeapStart() );

        D3D12_RENDER_TARGET_VIEW_DESC view_desc {};
        view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        view_desc.Format        = image_desc.Format;

        device->device->CreateRenderTargetView( image->image,
                                                nullptr,
                                                rtv_heap_handle );

        image->image_view = rtv_heap_handle;
        rtv_heap_handle.ptr += ( 1 * device->rtv_descriptor_size );
    }
    else
    {
        D3D12_CPU_DESCRIPTOR_HANDLE srv_uav_heap_handle(
            device->cbv_srv_uav_heap->GetCPUDescriptorHandleForHeapStart() );

        D3D12_SHADER_RESOURCE_VIEW_DESC view_desc {};
        view_desc.ViewDimension       = D3D12_SRV_DIMENSION_TEXTURE2D;
        view_desc.Format              = image_desc.Format;
        view_desc.Texture2D.MipLevels = 1;

        device->device->CreateShaderResourceView( image->image,
                                                  nullptr,
                                                  srv_uav_heap_handle );
        image->image_view = srv_uav_heap_handle;
        srv_uav_heap_handle.ptr += ( 1 * device->cbv_srv_uav_descriptor_size );
    }
}

void d3d12_destroy_image( const Device* device, Image* iimage )
{
    FT_ASSERT( iimage );

	FT_FROM_HANDLE( image, iimage, D3D12Image );

    if ( image->allocation )
    {
        image->allocation->Release();
    }
    image->image->Release();
    operator delete( image, std::nothrow );
}

void d3d12_create_descriptor_set( const Device*            idevice,
                                  const DescriptorSetDesc* desc,
                                  DescriptorSet**          p )
{
}

void d3d12_destroy_descriptor_set( const Device* idevice, DescriptorSet* iset )
{
}

void d3d12_update_descriptor_set( const Device*          idevice,
                                  DescriptorSet*         iset,
                                  u32                    count,
                                  const DescriptorWrite* writes )
{
}

void d3d12_create_ui_context( CommandBuffer* icmd,
                              const UiDesc*  desc,
                              UiContext**    p )
{
    FT_ASSERT( p );

	FT_FROM_HANDLE( device, desc->device, D3D12Device );
	FT_FROM_HANDLE( render_pass, desc->render_pass, D3D12RenderPass );

    auto context              = new ( std::nothrow ) D3D12UiContext {};
    context->interface.handle = context;
    *p                        = &context->interface;

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

    D3D12_ASSERT( device->device->CreateDescriptorHeap(
        &heap_desc,
        IID_PPV_ARGS( &context->cbv_srv_heap ) ) );

    ImGui_ImplSDL2_InitForD3D( ( SDL_Window* ) desc->window->handle );
    ImGui_ImplDX12_Init(
        device->device,
        desc->in_fly_frame_count,
        render_pass->color_formats[ 0 ],
        context->cbv_srv_heap,
        context->cbv_srv_heap->GetCPUDescriptorHandleForHeapStart(),
        context->cbv_srv_heap->GetGPUDescriptorHandleForHeapStart() );
}

void d3d12_destroy_ui_context( const Device* idevice, UiContext* icontext )
{
    FT_ASSERT( icontext );

	FT_FROM_HANDLE( context, icontext, D3D12UiContext );

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    context->cbv_srv_heap->Release();
    ImGui::DestroyContext();
    operator delete( context, std::nothrow );
}

void d3d12_ui_begin_frame()
{
    ImGui_ImplDX12_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}

void d3d12_ui_end_frame( UiContext* icontext, CommandBuffer* icmd )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );
	FT_FROM_HANDLE( context, icontext, D3D12UiContext );

    ImGui::Render();
    cmd->command_list->SetDescriptorHeaps( 1, &context->cbv_srv_heap );
    ImGui_ImplDX12_RenderDrawData( ImGui::GetDrawData(), cmd->command_list );

    ImGuiIO& io = ImGui::GetIO();
    if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

std::vector<char> d3d12_read_shader( const std::string& shader_name )
{
    return fs::read_file_binary( fs::get_shaders_directory() + "d3d12/" +
                                 shader_name + ".bin" );
}

void d3d12_create_renderer_backend( const RendererBackendDesc* desc,
                                    RendererBackend**          p )
{
    FT_ASSERT( p );

    destroy_renderer_backend      = d3d12_destroy_renderer_backend;
    create_device                 = d3d12_create_device;
    destroy_device                = d3d12_destroy_device;
    create_queue                  = d3d12_create_queue;
    destroy_queue                 = d3d12_destroy_queue;
    queue_wait_idle               = d3d12_queue_wait_idle;
    queue_submit                  = d3d12_queue_submit;
    immediate_submit              = d3d12_immediate_submit;
    queue_present                 = d3d12_queue_present;
    create_semaphore              = d3d12_create_semaphore;
    destroy_semaphore             = d3d12_destroy_semaphore;
    create_fence                  = d3d12_create_fence;
    destroy_fence                 = d3d12_destroy_fence;
    wait_for_fences               = d3d12_wait_for_fences;
    reset_fences                  = d3d12_reset_fences;
    create_swapchain              = d3d12_create_swapchain;
    resize_swapchain              = d3d12_resize_swapchain;
    destroy_swapchain             = d3d12_destroy_swapchain;
    create_command_pool           = d3d12_create_command_pool;
    destroy_command_pool          = d3d12_destroy_command_pool;
    create_command_buffers        = d3d12_create_command_buffers;
    free_command_buffers          = d3d12_free_command_buffers;
    destroy_command_buffers       = d3d12_destroy_command_buffers;
    begin_command_buffer          = d3d12_begin_command_buffer;
    end_command_buffer            = d3d12_end_command_buffer;
    acquire_next_image            = d3d12_acquire_next_image;
    create_render_pass            = d3d12_create_render_pass;
    update_render_pass            = d3d12_update_render_pass;
    destroy_render_pass           = d3d12_destroy_render_pass;
    create_shader                 = d3d12_create_shader;
    destroy_shader                = d3d12_destroy_shader;
    create_descriptor_set_layout  = d3d12_create_descriptor_set_layout;
    destroy_descriptor_set_layout = d3d12_destroy_descriptor_set_layout;
    create_compute_pipeline       = d3d12_create_compute_pipeline;
    create_graphics_pipeline      = d3d12_create_graphics_pipeline;
    destroy_pipeline              = d3d12_destroy_pipeline;
    create_buffer                 = d3d12_create_buffer;
    destroy_buffer                = d3d12_destroy_buffer;
    map_memory                    = d3d12_map_memory;
    unmap_memory                  = d3d12_unmap_memory;
    create_sampler                = d3d12_create_sampler;
    destroy_sampler               = d3d12_destroy_sampler;
    create_image                  = d3d12_create_image;
    destroy_image                 = d3d12_destroy_image;
    create_descriptor_set         = d3d12_create_descriptor_set;
    destroy_descriptor_set        = d3d12_destroy_descriptor_set;
    update_descriptor_set         = d3d12_update_descriptor_set;
    create_ui_context             = d3d12_create_ui_context;
    destroy_ui_context            = d3d12_destroy_ui_context;
    ui_begin_frame                = d3d12_ui_begin_frame;
    ui_end_frame                  = d3d12_ui_end_frame;
    cmd_begin_render_pass         = d3d12_cmd_begin_render_pass;
    cmd_end_render_pass           = d3d12_cmd_end_render_pass;
    cmd_barrier                   = d3d12_cmd_barrier;
    cmd_set_scissor               = d3d12_cmd_set_scissor;
    cmd_set_viewport              = d3d12_cmd_set_viewport;
    cmd_bind_pipeline             = d3d12_cmd_bind_pipeline;
    cmd_draw                      = d3d12_cmd_draw;
    cmd_draw_indexed              = d3d12_cmd_draw_indexed;
    cmd_bind_vertex_buffer        = d3d12_cmd_bind_vertex_buffer;
    cmd_bind_index_buffer_u16     = d3d12_cmd_bind_index_buffer_u16;
    cmd_bind_index_buffer_u32     = d3d12_cmd_bind_index_buffer_u32;
    cmd_copy_buffer               = d3d12_cmd_copy_buffer;
    cmd_copy_buffer_to_image      = d3d12_cmd_copy_buffer_to_image;
    cmd_bind_descriptor_set       = d3d12_cmd_bind_descriptor_set;
    cmd_dispatch                  = d3d12_cmd_dispatch;
    cmd_push_constants            = d3d12_cmd_push_constants;
    cmd_blit_image                = d3d12_cmd_blit_image;
    cmd_clear_color_image         = d3d12_cmd_clear_color_image;
    cmd_draw_indexed_indirect     = d3d12_cmd_draw_indexed_indirect;

    read_shader = d3d12_read_shader;

    auto backend              = new ( std::nothrow ) D3D12RendererBackend {};
    backend->interface.handle = backend;
    *p                        = &backend->interface;

#ifdef FLUENT_DEBUG
    D3D12_ASSERT(
        D3D12GetDebugInterface( IID_PPV_ARGS( &backend->debug_controller ) ) );
    backend->debug_controller->EnableDebugLayer();
#endif
    D3D12_ASSERT( CreateDXGIFactory1( IID_PPV_ARGS( &backend->factory ) ) );
}
} // namespace fluent
#endif
