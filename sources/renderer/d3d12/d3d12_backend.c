#ifdef D3D12_BACKEND

#define INCLUDE_WIN_DEFINES
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_sdl.h>
#include <tinyimageformat_apis.h>
#include <tinyimageformat_query.h>
#include "core/window.hpp"
#include "fs/fs.hpp"
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

static D3D12UiContext ui_context;

static inline DXGI_FORMAT
to_dxgi_image_format( enum Format format )
{
	switch ( format )
	{
	case FT_FORMAT_B8G8R8A8_SRGB:
	{
		format = FT_FORMAT_B8G8R8A8_UNORM;
		break;
	}
	case FT_FORMAT_R8G8B8A8_SRGB:
	{
		format = FT_FORMAT_R8G8B8A8_UNORM;
		break;
	}
	}
	return static_cast<DXGI_FORMAT>( TinyImageFormat_ToDXGI_FORMAT(
	    static_cast<TinyImageFormat>( format ) ) );
}

static inline DXGI_FORMAT
to_dxgi_format( enum Format format )
{
	return static_cast<DXGI_FORMAT>( TinyImageFormat_ToDXGI_FORMAT(
	    static_cast<TinyImageFormat>( format ) ) );
}

static inline D3D12_COMMAND_LIST_TYPE
to_d3d12_command_list_type( QueueType type )
{
	switch ( type )
	{
	case FT_QUEUE_TYPE_GRAPHICS: return D3D12_COMMAND_LIST_TYPE_DIRECT;
	case FT_QUEUE_TYPE_COMPUTE: return D3D12_COMMAND_LIST_TYPE_COMPUTE;
	case FT_QUEUE_TYPE_TRANSFER: return D3D12_COMMAND_LIST_TYPE_COPY;
	default: FT_ASSERT( false ); return D3D12_COMMAND_LIST_TYPE( -1 );
	}
}

static inline D3D12_RESOURCE_STATES
to_d3d12_resource_state( enum ResourceState state )
{
	switch ( state )
	{
	case FT_RESOURCE_STATE_UNDEFINED: return D3D12_RESOURCE_STATE_COMMON;
	case FT_RESOURCE_STATE_GENERAL:
		return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	case FT_RESOURCE_STATE_COLOR_ATTACHMENT:
		return D3D12_RESOURCE_STATE_RENDER_TARGET;
	case FT_RESOURCE_STATE_DEPTH_STENCIL_WRITE:
		return D3D12_RESOURCE_STATE_DEPTH_WRITE;
	case FT_RESOURCE_STATE_DEPTH_STENCIL_READ_ONLY:
		return D3D12_RESOURCE_STATE_DEPTH_READ;
	case FT_RESOURCE_STATE_SHADER_READ_ONLY:
		return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	case FT_RESOURCE_STATE_TRANSFER_SRC:
		return D3D12_RESOURCE_STATE_COPY_SOURCE;
	case FT_RESOURCE_STATE_TRANSFER_DST: return D3D12_RESOURCE_STATE_COPY_DEST;
	case FT_RESOURCE_STATE_PRESENT: return D3D12_RESOURCE_STATE_PRESENT;
	default: FT_ASSERT( false ); return D3D12_RESOURCE_STATES( -1 );
	}
}

static inline u32
to_d3d12_sample_count( SampleCount sample_count )
{
	switch ( sample_count )
	{
	case SampleCount::E1: return 1;
	case SampleCount::E2: return 2;
	case SampleCount::E4: return 4;
	case SampleCount::E8: return 8;
	case SampleCount::E16: return 16;
	case SampleCount::E32: return 32;
	case SampleCount::E64: return 64;
	default: FT_ASSERT( false ); return -1;
	}
}

static inline D3D12_FILL_MODE
to_d3d12_fill_mode( PolygonMode mode )
{
	switch ( mode )
	{
	case FT_POLYGON_MODE_FILL: return D3D12_FILL_MODE_SOLID;
	case FT_POLYGON_MODE_LINE: return D3D12_FILL_MODE_WIREFRAME;
	default: FT_ASSERT( false ); return D3D12_FILL_MODE( -1 );
	}
}

static inline D3D12_CULL_MODE
to_d3d12_cull_mode( CullMode mode )
{
	switch ( mode )
	{
	case FT_CULL_MODE_NONE: return D3D12_CULL_MODE_NONE;
	case FT_CULL_MODE_BACK: return D3D12_CULL_MODE_BACK;
	case FT_CULL_MODE_FRONT: return D3D12_CULL_MODE_FRONT;
	default: FT_ASSERT( false ); return D3D12_CULL_MODE( -1 );
	}
}

static inline b32
to_d3d12_front_face( FrontFace front_face )
{
	if ( front_face == FT_FRONT_FACE_COUNTER_CLOCKWISE )
	{
		return true;
	}
	else
	{
		return false;
	}
}

static inline D3D12_PRIMITIVE_TOPOLOGY_TYPE
to_d3d12_primitive_topology_type( PrimitiveTopology topology )
{
	switch ( topology )
	{
	case FT_PRIMITIVE_TOPOLOGY_POINT_LIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case FT_PRIMITIVE_TOPOLOGY_LINE_LIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case FT_PRIMITIVE_TOPOLOGY_LINE_STRIP:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case FT_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case FT_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	default: FT_ASSERT( false ); return D3D12_PRIMITIVE_TOPOLOGY_TYPE( -1 );
	}
}

static inline D3D12_PRIMITIVE_TOPOLOGY
to_d3d12_primitive_topology( PrimitiveTopology topology )
{
	switch ( topology )
	{
	case FT_PRIMITIVE_TOPOLOGY_POINT_LIST:
		return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	case FT_PRIMITIVE_TOPOLOGY_LINE_LIST:
		return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	case FT_PRIMITIVE_TOPOLOGY_LINE_STRIP:
		return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case FT_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case FT_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	default: FT_ASSERT( false ); return D3D12_PRIMITIVE_TOPOLOGY( -1 );
	}
}

static inline D3D12_HEAP_TYPE
to_d3d12_heap_type( MemoryUsage usage )
{
	switch ( usage )
	{
	case FT_MEMORY_USAGE_GPU_ONLY: return D3D12_HEAP_TYPE_DEFAULT;
	case FT_MEMORY_USAGE_CPU_ONLY: return D3D12_HEAP_TYPE_UPLOAD;
	case FT_MEMORY_USAGE_CPU_TO_GPU: return D3D12_HEAP_TYPE_UPLOAD;
	case FT_MEMORY_USAGE_GPU_TO_CPU: return D3D12_HEAP_TYPE_READBACK;
	case FT_MEMORY_USAGE_CPU_COPY: return D3D12_HEAP_TYPE_UPLOAD;
	default: FT_ASSERT( false ); return D3D12_HEAP_TYPE( -1 );
	}
}

static inline D3D12_FILTER_TYPE
to_d3d12_filter_type( enum Filter filter )
{
	switch ( filter )
	{
	case FT_FILTER_NEAREST: return D3D12_FILTER_TYPE_POINT;
	case FT_FILTER_LINEAR: return D3D12_FILTER_TYPE_LINEAR;
	default: FT_ASSERT( false ); return D3D12_FILTER_TYPE( -1 );
	}
}

static inline D3D12_FILTER
to_d3d12_filter( enum Filter       min_filter,
                 enum Filter       mag_filter,
                 SamplerMipmapMode mip_map_mode,
                 b32               anisotropy,
                 b32               comparison_filter_enabled )
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

static inline D3D12_TEXTURE_ADDRESS_MODE
to_d3d12_address_mode( SamplerAddressMode mode )
{
	switch ( mode )
	{
	case FT_SAMPLER_ADDRESS_MODE_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	case FT_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	case FT_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case FT_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
		return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	default: FT_ASSERT( false ); return D3D12_TEXTURE_ADDRESS_MODE( -1 );
	}
}

static inline D3D12_COMPARISON_FUNC
to_d3d12_comparison_func( CompareOp op )
{
	switch ( op )
	{
	case FT_COMPARE_OP_NEVER: return D3D12_COMPARISON_FUNC_NEVER;
	case FT_COMPARE_OP_LESS: return D3D12_COMPARISON_FUNC_LESS;
	case FT_COMPARE_OP_EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
	case FT_COMPARE_OP_LESS_OR_EQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case FT_COMPARE_OP_GREATER: return D3D12_COMPARISON_FUNC_GREATER;
	case FT_COMPARE_OP_NOT_EQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case FT_COMPARE_OP_GREATER_OR_EQUAL:
		return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case FT_COMPARE_OP_ALWAYS: return D3D12_COMPARISON_FUNC_ALWAYS;
	default: FT_ASSERT( false ); return D3D12_COMPARISON_FUNC( -1 );
	}
}

static inline D3D12_DESCRIPTOR_RANGE_TYPE
to_d3d12_descriptor_range_type( DescriptorType type )
{
	switch ( type )
	{
	case FT_DESCRIPTOR_TYPE_SAMPLER: return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	case FT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
	case FT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
	case FT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
		return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	default: return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	}
}

static inline D3D12_SHADER_VISIBILITY
to_d3d12_shader_visibility( ShaderStage stage )
{
	switch ( stage )
	{
	case FT_SHADER_STAGE_VERTEX: return D3D12_SHADER_VISIBILITY_VERTEX;
	case FT_SHADER_STAGE_TESSELLATION_CONTROL:
		return D3D12_SHADER_VISIBILITY_HULL;
	case FT_SHADER_STAGE_TESSELLATION_EVALUATION:
		return D3D12_SHADER_VISIBILITY_DOMAIN;
	case FT_SHADER_STAGE_GEOMETRY: return D3D12_SHADER_VISIBILITY_GEOMETRY;
	case FT_SHADER_STAGE_FRAGMENT: return D3D12_SHADER_VISIBILITY_PIXEL;
	default: FT_ASSERT( false ); return D3D12_SHADER_VISIBILITY( -1 );
	}
}

void
d3d12_destroy_renderer_backend( struct RendererBackend* ibackend )
{
	FT_ASSERT( ibackend );

	FT_FROM_HANDLE( backend, ibackend, D3D12RendererBackend );

	backend->factory->Release();
#ifdef FLUENT_DEBUG
	backend->debug_controller->Release();
#endif
	std::free( backend );
}

void
d3d12_create_device( const struct RendererBackend* ibackend,
                     const struct DeviceInfo*      info,
                     struct Device**               p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( backend, ibackend, D3D12RendererBackend );

	FT_INIT_INTERNAL( device, *p, D3D12Device );

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

void
d3d12_destroy_device( struct Device* idevice )
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

	std::free( device );
}

void
d3d12_create_queue( const struct Device*    idevice,
                    const struct QueueInfo* info,
                    struct Queue**          p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, D3D12Device );

	FT_INIT_INTERNAL( queue, *p, D3D12Queue );

	D3D12_COMMAND_QUEUE_DESC queue_desc = {};
	queue_desc.Type  = to_d3d12_command_list_type( info->queue_type );
	queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	D3D12_ASSERT(
	    device->device->CreateCommandQueue( &queue_desc,
	                                        IID_PPV_ARGS( &queue->queue ) ) );
	D3D12_ASSERT(
	    device->device->CreateFence( 0,
	                                 D3D12_FENCE_FLAG_NONE,
	                                 IID_PPV_ARGS( &queue->fence ) ) );
}

void
d3d12_destroy_queue( struct Queue* iqueue )
{
	FT_ASSERT( iqueue );

	FT_FROM_HANDLE( queue, iqueue, D3D12Queue );

	queue->fence->Release();
	queue->queue->Release();
	std::free( queue );
}

void
d3d12_create_command_pool( const struct Device*          idevice,
                           const struct CommandPoolInfo* info,
                           struct CommandPool**          p )
{
	FT_ASSERT( idevice );
	FT_ASSERT( info->queue );
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, D3D12Device );

	FT_INIT_INTERNAL( command_pool, *p, D3D12CommandPool );

	command_pool->interface.queue = info->queue;

	D3D12_ASSERT( device->device->CreateCommandAllocator(
	    to_d3d12_command_list_type( command_pool->interface.queue->type ),
	    IID_PPV_ARGS( &command_pool->command_allocator ) ) );
}

void
d3d12_destroy_command_pool( const struct Device* idevice,
                            struct CommandPool*  icommand_pool )
{
	FT_ASSERT( icommand_pool );

	FT_FROM_HANDLE( command_pool, icommand_pool, D3D12CommandPool );

	command_pool->command_allocator->Release();
	std::free( command_pool );
}

void
d3d12_create_command_buffers( const struct Device*      idevice,
                              const struct CommandPool* icommand_pool,
                              u32                       count,
                              struct CommandBuffer**    icommand_buffers )
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

void
d3d12_free_command_buffers( const struct Device*      idevice,
                            const struct CommandPool* icommand_pool,
                            u32                       count,
                            struct CommandBuffer**    icommand_buffers )
{
	FT_ASSERT( false );
}

void
d3d12_destroy_command_buffers( const struct Device*      idevice,
                               const struct CommandPool* icommand_pool,
                               u32                       count,
                               struct CommandBuffer**    icommand_buffers )
{
	FT_ASSERT( icommand_buffers );

	for ( u32 i = 0; i < count; ++i )
	{
		FT_FROM_HANDLE( cmd, icommand_buffers[ i ], D3D12CommandBuffer );
		cmd->command_list->Release();
		std::free( cmd );
	}
}

void
d3d12_create_semaphore( const struct Device* idevice, struct Semaphore** p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, D3D12Device );

	FT_INIT_INTERNAL( semaphore, *p, D3D12Semaphore );

	device->device->CreateFence( 0,
	                             D3D12_FENCE_FLAG_NONE,
	                             IID_PPV_ARGS( &semaphore->fence ) );
}

void
d3d12_destroy_semaphore( const struct Device* idevice,
                         struct Semaphore*    isemaphore )
{
	FT_ASSERT( isemaphore );

	FT_FROM_HANDLE( semaphore, isemaphore, D3D12Semaphore );

	semaphore->fence->Release();
	std::free( semaphore );
}

void
d3d12_create_fence( const struct Device* idevice, struct Fence** p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, D3D12Device );

	FT_INIT_INTERNAL( fence, *p, D3D12Fence );

	device->device->CreateFence( 0,
	                             D3D12_FENCE_FLAG_NONE,
	                             IID_PPV_ARGS( &fence->fence ) );
	fence->event_handle = CreateEvent( nullptr, false, false, nullptr );
}

void
d3d12_destroy_fence( const struct Device* idevice, struct Fence* ifence )
{
	FT_ASSERT( ifence );

	FT_FROM_HANDLE( fence, ifence, D3D12Fence );

	CloseHandle( fence->event_handle );
	fence->fence->Release();
	std::free( fence );
}

void
d3d12_queue_wait_idle( const struct Queue* iqueue )
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

void
d3d12_queue_submit( const struct Queue*           iqueue,
                    const struct QueueSubmitInfo* info )
{
	FT_FROM_HANDLE( queue, iqueue, D3D12Queue );

	std::vector<ID3D12CommandList*> command_lists( info->command_buffer_count );

	for ( u32 i = 0; i < info->command_buffer_count; i++ )
	{
		FT_FROM_HANDLE( cmd, info->command_buffers[ i ], D3D12CommandBuffer );
		command_lists[ i ] = cmd->command_list;
	}

	queue->queue->ExecuteCommandLists( info->command_buffer_count,
	                                   command_lists.data() );
}

void
d3d12_immediate_submit( const struct Queue* iqueue, struct CommandBuffer* icmd )
{
	QueueSubmitInfo queue_submit_desc {};
	queue_submit_desc.command_buffer_count = 1;
	queue_submit_desc.command_buffers      = &icmd;
	queue_submit( iqueue, &queue_submit_desc );
	queue_wait_idle( iqueue );
}

void
d3d12_queue_present( const struct Queue*            iqueue,
                     const struct QueuePresentInfo* info )
{
	FT_FROM_HANDLE( queue, iqueue, D3D12Queue );
	FT_FROM_HANDLE( swapchain, info->swapchain, D3D12Swapchain );

	u32 sync_interval = info->swapchain->vsync ? 1 : 0;
	swapchain->swapchain->Present( sync_interval, 0 );
	for ( u32 i = 0; i < info->wait_semaphore_count; i++ )
	{
		FT_FROM_HANDLE( semaphore, info->wait_semaphores[ i ], D3D12Semaphore );

		queue->queue->Signal( semaphore->fence, semaphore->fence_value );
		semaphore->fence_value++;
	}
}

void
d3d12_wait_for_fences( const struct Device* idevice,
                       u32                  count,
                       struct Fence**       ifences )
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

void
d3d12_reset_fences( const struct Device* idevice,
                    u32                  count,
                    struct Fence**       ifences )
{
}

void
d3d12_create_swapchain( const struct Device*        idevice,
                        const struct SwapchainInfo* info,
                        struct Swapchain**          p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, D3D12Device );
	FT_FROM_HANDLE( queue, info->queue, D3D12Queue );

	auto swapchain              = new ( std::nothrow ) D3D12Swapchain {};
	swapchain->interface.handle = swapchain;
	*p                          = &swapchain->interface;

	swapchain->interface.width           = info->width;
	swapchain->interface.height          = info->height;
	swapchain->interface.format          = info->format;
	swapchain->interface.queue           = info->queue;
	swapchain->interface.min_image_count = info->min_image_count;
	swapchain->interface.image_count     = info->min_image_count;
	swapchain->interface.vsync           = info->vsync;
	swapchain->image_index               = 0;

	DXGI_SWAP_CHAIN_DESC swapchain_desc {};
	swapchain_desc.BufferDesc.Width  = swapchain->interface.width;
	swapchain_desc.BufferDesc.Height = swapchain->interface.height;
	swapchain_desc.BufferDesc.RefreshRate.Numerator   = 60;
	swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
	swapchain_desc.BufferDesc.enum Format =
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

	swapchain->interface.images = static_cast<struct Image**>(
	    std::calloc( swapchain->interface.image_count,
	                 sizeof( struct Image* ) ) );

	D3D12_CPU_DESCRIPTOR_HANDLE rtv_heap_handle(
	    device->rtv_heap->GetCPUDescriptorHandleForHeapStart() );

	for ( u32 i = 0; i < swapchain->interface.image_count; i++ )
	{
		FT_INIT_INTERNAL( image, swapchain->interface.images[ i ], D3D12Image );

		D3D12_ASSERT(
		    swapchain->swapchain->GetBuffer( i,
		                                     IID_PPV_ARGS( &image->image ) ) );

		image->interface.width  = swapchain->interface.width;
		image->interface.height = swapchain->interface.height;
		image->interface.depth  = 1;
		image->interface.format = swapchain->interface.format;

		D3D12_RENDER_TARGET_VIEW_DESC view_desc {};
		view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		view_desc.enum Format   = to_dxgi_format( swapchain->interface.format );

		device->device->CreateRenderTargetView( image->image,
		                                        &view_desc,
		                                        rtv_heap_handle );

		image->image_view = rtv_heap_handle;
		rtv_heap_handle.ptr += ( 1 * device->rtv_descriptor_size );
	}
}

void
d3d12_resize_swapchain( const struct Device* idevice,
                        struct Swapchain*    iswapchain,
                        u32                  width,
                        u32                  height )
{
}

void
d3d12_destroy_swapchain( const struct Device* idevice,
                         struct Swapchain*    iswapchain )
{
	FT_ASSERT( iswapchain );

	FT_FROM_HANDLE( swapchain, iswapchain, D3D12Swapchain );

	for ( u32 i = 0; i < swapchain->interface.image_count; i++ )
	{
		FT_FROM_HANDLE( image, swapchain->interface.images[ i ], D3D12Image );
		image->image->Release();
		std::free( image );
	}
	std::free( swapchain->interface.images );
	swapchain->swapchain->Release();
	std::free( swapchain );
}

void
d3d12_begin_command_buffer( const struct CommandBuffer* icmd )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );
	D3D12_ASSERT( cmd->command_list->Reset( cmd->command_allocator, nullptr ) );
}

void
d3d12_end_command_buffer( const struct CommandBuffer* icmd )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );
	D3D12_ASSERT( cmd->command_list->Close() );
}

void
d3d12_acquire_next_image( const struct Device*    idevice,
                          const struct Swapchain* iswapchain,
                          const struct Semaphore* isemaphore,
                          const struct Fence*     ifence,
                          u32*                    image_index )
{
	FT_FROM_HANDLE( swapchain, iswapchain, D3D12Swapchain );

	*image_index = swapchain->image_index;
	swapchain->image_index =
	    ( swapchain->image_index + 1 ) % swapchain->interface.image_count;
}

// void
// d3d12_create_render_pass( const struct Device*         idevice,
//                           const RenderPassInfo* info,
//                           RenderPass**          p )
//{
//	FT_ASSERT( p );
//
//	FT_INIT_INTERNAL( render_pass, *p, D3D12RenderPass );
//
//	render_pass->interface.color_attachment_count =
//	    info->color_attachment_count;
//
//	for ( u32 i = 0; i < info->color_attachment_count; i++ )
//	{
//		FT_FROM_HANDLE( image, info->color_attachments[ i ], D3D12Image );
//
//		render_pass->color_attachments[ i ] = image->image_view;
//		render_pass->color_formats[ i ] =
//		    to_dxgi_format( info->color_attachments[ i ]->format );
//	}
//
//	if ( info->depth_stencil )
//	{
//		FT_FROM_HANDLE( image, info->depth_stencil, D3D12Image );
//
//		render_pass->interface.has_depth_stencil = true;
//		render_pass->depth_stencil               = image->image_view;
//		render_pass->depth_format =
//		    to_dxgi_format( info->depth_stencil->format );
//	}
// }
//
// void
// d3d12_resize_render_pass( const struct Device*         idevice,
//                           RenderPass*           irender_pass,
//                           const RenderPassInfo* info )
//{
// }
//
// void
// d3d12_destroy_render_pass( const struct Device* idevice, RenderPass*
// irender_pass )
//{
//	FT_ASSERT( irender_pass );
//
//	FT_FROM_HANDLE( render_pass, irender_pass, D3D12RenderPass );
//
//	std::free( render_pass );
// }

void
d3d12_create_shader( const struct Device* idevice,
                     struct ShaderInfo*   info,
                     struct Shader**      p )
{
	FT_ASSERT( p );

	FT_INIT_INTERNAL( shader, *p, D3D12Shader );

	new ( &shader->interface.reflect_data.binding_map ) BindingMap();
	new ( &shader->interface.reflect_data.bindings ) Bindings();

	auto create_module = []( D3D12Shader*            shader,
	                         ShaderStage             stage,
	                         const ShaderModuleInfo& info )
	{
		if ( info.bytecode )
		{
			char* dst = static_cast<char*>( std::malloc( info.bytecode_size ) );
			const char* src = static_cast<const char*>( info.bytecode );

			std::memcpy( dst, src, info.bytecode_size );
			shader->bytecodes[ static_cast<u32>( stage ) ].BytecodeLength =
			    info.bytecode_size;
			shader->bytecodes[ static_cast<u32>( stage ) ].pShaderBytecode =
			    dst;
		}
	};

	create_module( shader, FT_SHADER_STAGE_COMPUTE, info->compute );
	create_module( shader, FT_SHADER_STAGE_VERTEX, info->vertex );
	create_module( shader,
	               FT_SHADER_STAGE_TESSELLATION_CONTROL,
	               info->tessellation_control );
	create_module( shader,
	               FT_SHADER_STAGE_TESSELLATION_EVALUATION,
	               info->tessellation_evaluation );
	create_module( shader, FT_SHADER_STAGE_GEOMETRY, info->geometry );
	create_module( shader, FT_SHADER_STAGE_FRAGMENT, info->fragment );

	dxil_reflect( idevice, info, *p );
}

void
d3d12_destroy_shader( const struct Device* idevice, struct Shader* ishader )
{
	FT_ASSERT( ishader );

	FT_FROM_HANDLE( shader, ishader, D3D12Shader );

	auto destroy_module = []( ShaderStage stage, D3D12Shader* shader )
	{
		if ( shader->bytecodes[ static_cast<u32>( stage ) ].pShaderBytecode )
		{
			// FIXME its not nice solution. this is const void*
			std::free( ( void* ) shader->bytecodes[ static_cast<u32>( stage ) ]
			               .pShaderBytecode );
		}
	};

	destroy_module( FT_SHADER_STAGE_COMPUTE, shader );
	destroy_module( FT_SHADER_STAGE_VERTEX, shader );
	destroy_module( FT_SHADER_STAGE_TESSELLATION_CONTROL, shader );
	destroy_module( FT_SHADER_STAGE_TESSELLATION_EVALUATION, shader );
	destroy_module( FT_SHADER_STAGE_GEOMETRY, shader );
	destroy_module( FT_SHADER_STAGE_FRAGMENT, shader );

	shader->interface.reflect_data.bindings.~vector();
	shader->interface.reflect_data.binding_map.~unordered_map();

	std::free( shader );
}

void
d3d12_create_descriptor_set_layout( const struct Device*         idevice,
                                    struct Shader*               ishader,
                                    struct DescriptorSetLayout** p )
{
	FT_ASSERT( p );
	FT_ASSERT( shader_count );

	FT_FROM_HANDLE( device, idevice, D3D12Device );
	FT_FROM_HANDLE( shader, ishader, D3D12Shader );

	FT_INIT_INTERNAL( descriptor_set_layout, *p, D3D12DescriptorSetLayout );

	new ( &descriptor_set_layout->interface.reflection_data.binding_map )
	    BindingMap();
	new ( &descriptor_set_layout->interface.reflection_data.bindings )
	    Bindings();

	descriptor_set_layout->interface.reflection_data =
	    shader->interface.reflect_data;

	std::vector<D3D12_ROOT_PARAMETER1>   tables;
	std::vector<D3D12_DESCRIPTOR_RANGE1> cbv_ranges;
	std::vector<D3D12_DESCRIPTOR_RANGE1> srv_uav_ranges;
	std::vector<D3D12_DESCRIPTOR_RANGE1> sampler_ranges;

	for ( u32 s = 0; s < static_cast<u32>( FT_SHADER_STAGE_COUNT ); ++s )
	{
		u32 cbv_range_count     = cbv_ranges.size();
		u32 srv_uav_range_count = srv_uav_ranges.size();
		u32 sampler_range_count = sampler_ranges.size();

		if ( shader->bytecodes[ s ].pShaderBytecode == nullptr )
		{
			continue;
		}

		for ( u32 b = 0;
		      b <
		      descriptor_set_layout->interface.reflection_data.binding_count;
		      ++b )
		{
			const auto& binding =
			    descriptor_set_layout->interface.reflection_data.bindings[ b ];
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
			    to_d3d12_shader_visibility( static_cast<ShaderStage>( s ) );
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
			    to_d3d12_shader_visibility( static_cast<ShaderStage>( s ) );
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
			    to_d3d12_shader_visibility( static_cast<ShaderStage>( s ) );
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

void
d3d12_destroy_descriptor_set_layout( const struct Device*        idevice,
                                     struct DescriptorSetLayout* ilayout )
{
	FT_ASSERT( ilayout );

	FT_FROM_HANDLE( layout, ilayout, D3D12DescriptorSetLayout );

	layout->root_signature->Release();

	layout->interface.reflection_data.bindings.~vector();
	layout->interface.reflection_data.binding_map.~unordered_map();

	std::free( layout );
}

void
d3d12_create_compute_pipeline( const struct Device*       idevice,
                               const struct PipelineInfo* info,
                               struct Pipeline**          p )
{
}

void
d3d12_create_graphics_pipeline( const struct Device*       idevice,
                                const struct PipelineInfo* info,
                                struct Pipeline**          p )
{
	FT_ASSERT( p );
	FT_ASSERT( info->descriptor_set_layout );
	FT_ASSERT( info->render_pass );
	FT_ASSERT( info->shader );

	FT_INIT_INTERNAL( pipeline, *p, D3D12Pipeline );

	FT_FROM_HANDLE( device, idevice, D3D12Device );
	FT_FROM_HANDLE( dsl,
	                info->descriptor_set_layout,
	                D3D12DescriptorSetLayout );
	FT_FROM_HANDLE( shader, info->shader, D3D12Shader );

	pipeline->interface.type = FT_PIPELINE_TYPE_GRAPHICS;
	pipeline->root_signature = dsl->root_signature;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_desc {};
	pipeline_desc.pRootSignature = dsl->root_signature;
	pipeline_desc.VS =
	    shader->bytecodes[ static_cast<u32>( FT_SHADER_STAGE_VERTEX ) ];
	pipeline_desc.PS =
	    shader->bytecodes[ static_cast<u32>( FT_SHADER_STAGE_FRAGMENT ) ];
	pipeline_desc.GS =
	    shader->bytecodes[ static_cast<u32>( FT_SHADER_STAGE_GEOMETRY ) ];
	// TODO: other stages

	const VertexLayout& vertex_layout = info->vertex_layout;

	D3D12_INPUT_LAYOUT_DESC  input_layout {};
	D3D12_INPUT_ELEMENT_DESC input_elements[ MAX_VERTEX_ATTRIBUTE_COUNT ];
	u32                      input_element_count = 0;

	for ( u32 i = 0; i < vertex_layout.attribute_info_count; i++ )
	{
		const auto& attribute     = vertex_layout.attribute_infos[ i ];
		auto&       input_element = input_elements[ input_element_count ];

		input_element.enum Format       = to_dxgi_format( attribute.format );
		input_element.InputSlot         = attribute.binding;
		input_element.AlignedByteOffset = attribute.offset;
		input_element.SemanticIndex     = 0;
		input_element.SemanticName      = "TODO";

		if ( vertex_layout.binding_infos[ attribute.binding ].input_rate ==
		     FT_VERTEX_INPUT_RATE_INSTANCE )
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
	    to_d3d12_fill_mode( info->rasterizer_info.polygon_mode );
	rasterizer_desc.CullMode =
	    to_d3d12_cull_mode( info->rasterizer_info.cull_mode );
	rasterizer_desc.FrontCounterClockwise =
	    to_d3d12_front_face( info->rasterizer_info.front_face );
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

	pipeline_desc.NumRenderTargets = info->color_attachment_count;
	for ( u32 i = 0; i < pipeline_desc.NumRenderTargets; i++ )
	{
		pipeline_desc.RTVFormats[ i ] =
		    to_dxgi_format( info->color_attachment_formats[ i ] );
	}

	if ( info->depth_stencil_format != FT_FORMAT_UNDEFINED )
	{
		pipeline_desc.DSVFormat = to_dxgi_format( info->depth_stencil_format );
	}

	pipeline_desc.InputLayout     = input_layout;
	pipeline_desc.RasterizerState = rasterizer_desc;
	pipeline_desc.PrimitiveTopologyType =
	    to_d3d12_primitive_topology_type( info->topology );
	pipeline->topology       = to_d3d12_primitive_topology( info->topology );
	pipeline_desc.BlendState = blend_desc;
	pipeline_desc.DepthStencilState.DepthEnable =
	    info->depth_state_info.depth_test;
	pipeline_desc.DepthStencilState.StencilEnable = false;
	pipeline_desc.SampleMask       = std::numeric_limits<u32>::max();
	pipeline_desc.SampleDesc.Count = 1;

	D3D12_ASSERT( device->device->CreateGraphicsPipelineState(
	    &pipeline_desc,
	    IID_PPV_ARGS( &pipeline->pipeline ) ) );
}

void
d3d12_destroy_pipeline( const struct Device* idevice,
                        struct Pipeline*     ipipeline )
{
	FT_ASSERT( ipipeline );

	FT_FROM_HANDLE( pipeline, ipipeline, D3D12Pipeline );

	pipeline->pipeline->Release();
	std::free( pipeline );
}

void
d3d12_cmd_begin_render_pass( const struct CommandBuffer*       icmd,
                             const struct RenderPassBeginInfo* info )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );

	D3D12_CPU_DESCRIPTOR_HANDLE color_attachments[ MAX_ATTACHMENTS_COUNT ];
	D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil;

	for ( u32 i = 0; i < info->color_attachment_count; i++ )
	{
		FT_FROM_HANDLE( image, info->color_attachments[ i ], D3D12Image );
		color_attachments[ i ] = image->image_view;
		cmd->command_list->ClearRenderTargetView( image->image_view,
		                                          info->clear_values[ i ].color,
		                                          0,
		                                          nullptr );
	}

	if ( info->depth_stencil )
	{
		FT_FROM_HANDLE( image, info->depth_stencil, D3D12Image );
		depth_stencil = image->image_view;
	}

	cmd->command_list->OMSetRenderTargets( info->color_attachment_count,
	                                       color_attachments,
	                                       false, // TODO:
	                                       info->depth_stencil ? &depth_stencil
	                                                           : nullptr );
}

void
d3d12_cmd_end_render_pass( const struct CommandBuffer* icmd )
{
}

void
d3d12_cmd_barrier( const struct CommandBuffer* icmd,
                   u32                         memory_barriers_count,
                   const struct MemoryBarrier* memory_barrier,
                   u32                         buffer_barriers_count,
                   const struct BufferBarrier* buffer_barriers,
                   u32                         image_barriers_count,
                   const struct ImageBarrier*  image_barriers )
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

void
d3d12_cmd_set_scissor( const struct CommandBuffer* icmd,
                       i32                         x,
                       i32                         y,
                       u32                         width,
                       u32                         height )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );

	D3D12_RECT rect;
	rect.left   = x;
	rect.top    = y;
	rect.right  = width;
	rect.bottom = height;

	cmd->command_list->RSSetScissorRects( 1, &rect );
}

void
d3d12_cmd_set_viewport( const struct CommandBuffer* icmd,
                        f32                         x,
                        f32                         y,
                        f32                         width,
                        f32                         height,
                        f32                         min_depth,
                        f32                         max_depth )
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

void
d3d12_cmd_bind_pipeline( const struct CommandBuffer* icmd,
                         const struct Pipeline*      ipipeline )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );
	FT_FROM_HANDLE( pipeline, ipipeline, D3D12Pipeline );

	cmd->command_list->IASetPrimitiveTopology( pipeline->topology );
	cmd->command_list->SetGraphicsRootSignature( pipeline->root_signature );
	cmd->command_list->SetPipelineState( pipeline->pipeline );
}

void
d3d12_cmd_draw( const struct CommandBuffer* icmd,
                u32                         vertex_count,
                u32                         instance_count,
                u32                         first_vertex,
                u32                         first_instance )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );

	cmd->command_list->DrawInstanced( vertex_count,
	                                  instance_count,
	                                  first_vertex,
	                                  first_instance );
}

void
d3d12_cmd_draw_indexed( const struct CommandBuffer* icmd,
                        u32                         index_count,
                        u32                         instance_count,
                        u32                         first_index,
                        i32                         vertex_offset,
                        u32                         first_instance )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );

	cmd->command_list->DrawIndexedInstanced( index_count,
	                                         instance_count,
	                                         first_index,
	                                         vertex_offset,
	                                         first_instance );
}

void
d3d12_cmd_bind_vertex_buffer( const struct CommandBuffer* icmd,
                              const struct Buffer*        ibuffer,
                              const u64                   offset )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );
	FT_FROM_HANDLE( buffer, ibuffer, D3D12Buffer );

	D3D12_VERTEX_BUFFER_VIEW buffer_view {};
	buffer_view.BufferLocation = buffer->buffer->GetGPUVirtualAddress();
	buffer_view.SizeInBytes    = buffer->interface.size;
	buffer_view.StrideInBytes  = 4 * sizeof( float );
	cmd->command_list->IASetVertexBuffers( 0, 1, &buffer_view );
}

void
d3d12_cmd_bind_index_buffer_u16( const struct CommandBuffer* cmd,
                                 const struct Buffer*        buffer,
                                 const u64                   offset )
{
}

void
d3d12_cmd_bind_index_buffer_u32( const struct CommandBuffer* cmd,
                                 const struct Buffer*        buffer,
                                 u64                         offset )
{
}

void
d3d12_cmd_copy_buffer( const struct CommandBuffer* icmd,
                       const struct Buffer*        isrc,
                       u64                         src_offset,
                       struct Buffer*              idst,
                       u64                         dst_offset,
                       u64                         size )
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

void
d3d12_cmd_copy_buffer_to_image( const struct CommandBuffer* icmd,
                                const struct Buffer*        isrc,
                                u64                         src_offset,
                                struct Image*               idst )
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
	src_copy_location.PlacedFootprint.Footprint.enum Format =
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

void
d3d12_cmd_dispatch( const struct CommandBuffer* cmd,
                    u32                         group_count_x,
                    u32                         group_count_y,
                    u32                         group_count_z )
{
}

void
d3d12_cmd_push_constants( const struct CommandBuffer* cmd,
                          const struct Pipeline*      pipeline,
                          u64                         offset,
                          u64                         size,
                          const void*                 data )
{
}

void
d3d12_cmd_blit_image( const struct CommandBuffer* cmd,
                      const struct Image*         src,
                      enum ResourceState          src_state,
                      struct Image*               dst,
                      enum ResourceState          dst_state,
                      enum Filter                 filter )
{
}

void
d3d12_cmd_clear_color_image( const struct CommandBuffer* cmd,
                             struct Image*               image,
                             Vector4                     color )
{
}

void
d3d12_cmd_draw_indexed_indirect( const struct CommandBuffer* cmd,
                                 const struct Buffer*        buffer,
                                 u64                         offset,
                                 u32                         draw_count,
                                 u32                         stride )
{
}

void
d3d12_cmd_bind_descriptor_set( const struct CommandBuffer* cmd,
                               u32                         first_set,
                               const struct DescriptorSet* set,
                               const struct Pipeline*      pipeline )
{
}

void
d3d12_create_buffer( const struct Device*     idevice,
                     const struct BufferInfo* info,
                     struct Buffer**          p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, D3D12Device );

	FT_INIT_INTERNAL( buffer, *p, D3D12Buffer );

	buffer->interface.size            = info->size;
	buffer->interface.descriptor_type = info->descriptor_type;
	buffer->interface.memory_usage    = info->memory_usage;

	D3D12_RESOURCE_DESC buffer_desc {};
	buffer_desc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
	buffer_desc.Alignment        = 65536ul;
	buffer_desc.Width            = buffer->interface.size;
	buffer_desc.Height           = 1u;
	buffer_desc.DepthOrArraySize = 1;
	buffer_desc.MipLevels        = 1;
	buffer_desc.enum Format      = DXGI_FORMAT_UNKNOWN;
	buffer_desc.SampleDesc       = { 1u, 0u };
	buffer_desc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	buffer_desc.Flags            = D3D12_RESOURCE_FLAG_NONE;

	D3D12MA::ALLOCATION_DESC alloc_desc = {};
	alloc_desc.HeapType = to_d3d12_heap_type( info->memory_usage );

	D3D12_ASSERT(
	    device->allocator->CreateResource( &alloc_desc,
	                                       &buffer_desc,
	                                       D3D12_RESOURCE_STATE_GENERIC_READ,
	                                       nullptr,
	                                       &buffer->allocation,
	                                       IID_PPV_ARGS( &buffer->buffer ) ) );
}

void
d3d12_destroy_buffer( const struct Device* idevice, struct Buffer* ibuffer )
{
	FT_ASSERT( ibuffer );

	FT_FROM_HANDLE( buffer, ibuffer, D3D12Buffer );
	buffer->allocation->Release();
	buffer->buffer->Release();
	std::free( buffer );
}

void*
d3d12_map_memory( const struct Device* idevice, struct Buffer* ibuffer )
{
	FT_ASSERT( ibuffer != nullptr );
	FT_ASSERT( ibuffer->mapped_memory == nullptr );

	FT_FROM_HANDLE( buffer, ibuffer, D3D12Buffer );

	D3D12_ASSERT(
	    buffer->buffer->Map( 0, nullptr, &buffer->interface.mapped_memory ) );
	return buffer->interface.mapped_memory;
}

void
d3d12_unmap_memory( const struct Device* idevice, struct Buffer* ibuffer )
{
	FT_ASSERT( ibuffer );
	FT_ASSERT( ibuffer->mapped_memory );

	FT_FROM_HANDLE( buffer, ibuffer, D3D12Buffer );

	buffer->buffer->Unmap( 0, nullptr );
	buffer->interface.mapped_memory = nullptr;
}

void
d3d12_create_sampler( const struct Device*      idevice,
                      const struct SamplerInfo* info,
                      struct Sampler**          p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, D3D12Device );

	FT_INIT_INTERNAL( sampler, *p, D3D12Sampler );

	D3D12_SAMPLER_DESC sampler_desc {};
	sampler_desc.enum Filter = to_d3d12_filter( info->min_filter,
	                                            info->mag_filter,
	                                            info->mipmap_mode,
	                                            info->anisotropy_enable,
	                                            info->compare_enable );
	sampler_desc.AddressU    = to_d3d12_address_mode( info->address_mode_u );
	sampler_desc.AddressV    = to_d3d12_address_mode( info->address_mode_v );
	sampler_desc.AddressW    = to_d3d12_address_mode( info->address_mode_w );
	sampler_desc.MipLODBias  = info->mip_lod_bias;
	sampler_desc.MaxAnisotropy =
	    static_cast<u32>( info->max_anisotropy ); // ??? TODO
	sampler_desc.ComparisonFunc = to_d3d12_comparison_func( info->compare_op );
	sampler_desc.MinLOD         = info->min_lod;
	sampler_desc.MaxLOD         = info->max_lod;

	D3D12_CPU_DESCRIPTOR_HANDLE sampler_heap_handle(
	    device->sampler_heap->GetCPUDescriptorHandleForHeapStart() );
	device->device->CreateSampler( &sampler_desc, sampler_heap_handle );
	sampler->handle = sampler_heap_handle;
	sampler_heap_handle.ptr += ( 1 * device->sampler_descriptor_size );
}

void
d3d12_destroy_sampler( const struct Device* device, struct Sampler* isampler )
{
	FT_ASSERT( isampler );

	FT_FROM_HANDLE( sampler, isampler, D3D12Sampler );

	std::free( sampler );
}

void
d3d12_create_image( const struct Device*    idevice,
                    const struct ImageInfo* info,
                    struct Image**          p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, D3D12Device );

	FT_INIT_INTERNAL( image, *p, D3D12Image );

	image->interface.width           = info->width;
	image->interface.height          = info->height;
	image->interface.depth           = info->depth;
	image->interface.mip_level_count = info->mip_levels;
	image->interface.format          = info->format;
	image->interface.sample_count    = info->sample_count;

	D3D12_RESOURCE_DESC image_desc = {};
	// TODO:
	image_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	image_desc.Alignment        = 0;
	image_desc.Width            = image->interface.width;
	image_desc.Height           = image->interface.height;
	image_desc.DepthOrArraySize = image->interface.depth;
	image_desc.MipLevels        = image->interface.mip_level_count;
	image_desc.enum Format = to_dxgi_image_format( image->interface.format );
	image_desc.SampleDesc.Count =
	    to_d3d12_sample_count( image->interface.sample_count );
	// TODO:
	image_desc.SampleDesc.Quality = 0;
	image_desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	if ( format_has_depth_aspect( info->format ) ||
	     format_has_stencil_aspect( info->format ) )
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
		view_desc.enum Format        = image_desc.enum Format;
		view_desc.Texture2D.MipSlice = 0;
		device->device->CreateDepthStencilView( image->image,
		                                        nullptr,
		                                        dsv_heap_handle );
		image->image_view = dsv_heap_handle;
		dsv_heap_handle.ptr += ( 1 * device->dsv_descriptor_size );
	}
	else if ( info->descriptor_type == FT_DESCRIPTOR_TYPE_COLOR_ATTACHMENT )
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtv_heap_handle(
		    device->rtv_heap->GetCPUDescriptorHandleForHeapStart() );

		D3D12_RENDER_TARGET_VIEW_DESC view_desc {};
		view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		view_desc.enum Format   = image_desc.enum Format;

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
		view_desc.enum Format         = image_desc.enum Format;
		view_desc.Texture2D.MipLevels = 1;

		device->device->CreateShaderResourceView( image->image,
		                                          nullptr,
		                                          srv_uav_heap_handle );
		image->image_view = srv_uav_heap_handle;
		srv_uav_heap_handle.ptr += ( 1 * device->cbv_srv_uav_descriptor_size );
	}
}

void
d3d12_destroy_image( const struct Device* device, struct Image* iimage )
{
	FT_ASSERT( iimage );

	FT_FROM_HANDLE( image, iimage, D3D12Image );

	if ( image->allocation )
	{
		image->allocation->Release();
	}
	image->image->Release();
	std::free( image );
}

void
d3d12_create_descriptor_set( const struct Device*            idevice,
                             const struct DescriptorSetInfo* info,
                             struct DescriptorSet**          p )
{
}

void
d3d12_destroy_descriptor_set( const struct Device*  idevice,
                              struct DescriptorSet* iset )
{
}

void
d3d12_update_descriptor_set( const struct Device*          idevice,
                             struct DescriptorSet*         iset,
                             u32                           count,
                             const struct DescriptorWrite* writes )
{
}

void
d3d12_init_ui( const UiInfo* info )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, info->device, D3D12Device );

	ImGui::CreateContext();
	auto& io = ImGui::GetIO();
	( void ) io;
	if ( info->docking )
	{
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	}

	if ( info->viewports )
	{
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	}

	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.NumDescriptors = 1;
	heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	D3D12_ASSERT( device->device->CreateDescriptorHeap(
	    &heap_desc,
	    IID_PPV_ARGS( &ui_context.cbv_srv_heap ) ) );

	ImGui_ImplSDL2_InitForD3D( ( SDL_Window* ) info->window->handle );
	ImGui_ImplDX12_Init(
	    device->device,
	    info->in_fly_frame_count,
	    to_dxgi_format( info->color_attachment_formats[ 0 ] ),
	    ui_context.cbv_srv_heap,
	    ui_context.cbv_srv_heap->GetCPUDescriptorHandleForHeapStart(),
	    ui_context.cbv_srv_heap->GetGPUDescriptorHandleForHeapStart() );
}

void
d3d12_ui_upload_resources( struct CommandBuffer* cmd )
{
}

void
d3d12_ui_destroy_upload_objects()
{
}

void
d3d12_shutdown_ui( const struct Device* idevice )
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ui_context.cbv_srv_heap->Release();
	ImGui::DestroyContext();
}

void
d3d12_ui_begin_frame( struct CommandBuffer* icmd )
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}

void
d3d12_ui_end_frame( struct CommandBuffer* icmd )
{
	FT_FROM_HANDLE( cmd, icmd, D3D12CommandBuffer );

	ImGui::Render();
	cmd->command_list->SetDescriptorHeaps( 1, &ui_context.cbv_srv_heap );
	ImGui_ImplDX12_RenderDrawData( ImGui::GetDrawData(), cmd->command_list );

	ImGuiIO& io = ImGui::GetIO();
	if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

std::vector<char>
d3d12_read_shader( const std::string& shader_name )
{
	return fs::read_file_binary( fs::get_shaders_directory() + "d3d12/" +
	                             shader_name + ".bin" );
}

void
d3d12_create_renderer_backend( const RendererBackendInfo* info,
                               struct RendererBackend**   p )
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
	init_ui                       = d3d12_init_ui;
	ui_upload_resources           = d3d12_ui_upload_resources;
	ui_destroy_upload_objects     = d3d12_ui_destroy_upload_objects;
	shutdown_ui                   = d3d12_shutdown_ui;
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

	FT_INIT_INTERNAL( backend, *p, D3D12RendererBackend );

#ifdef FLUENT_DEBUG
	D3D12_ASSERT(
	    D3D12GetDebugInterface( IID_PPV_ARGS( &backend->debug_controller ) ) );
	backend->debug_controller->EnableDebugLayer();
#endif
	D3D12_ASSERT( CreateDXGIFactory1( IID_PPV_ARGS( &backend->factory ) ) );
}
} // namespace fluent
#endif