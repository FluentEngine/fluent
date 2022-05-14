#ifdef METAL_BACKEND

#define METAL_BACKEND_INCLUDE_OBJC
#include <unordered_map>
#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_metal.h>
#include <imgui_impl_sdl.h>
#include <tinyimageformat_apis.h>
#include "core/application.hpp"
#include "fs/fs.hpp"
#include "metal_backend.hpp"

namespace fluent
{

static inline MTLPixelFormat
to_mtl_format( Format format )
{
	return static_cast<MTLPixelFormat>( TinyImageFormat_ToMTLPixelFormat(
	    static_cast<TinyImageFormat>( format ) ) );
}

static inline MTLLoadAction
to_mtl_load_action( AttachmentLoadOp load_op )
{
	switch ( load_op )
	{
	case AttachmentLoadOp::LOAD: return MTLLoadActionLoad;
	case AttachmentLoadOp::CLEAR: return MTLLoadActionClear;
	case AttachmentLoadOp::DONT_CARE: return MTLLoadActionDontCare;
	default: FT_ASSERT( false ); return MTLLoadAction( -1 );
	}
}

static inline u32
to_mtl_sample_count( SampleCount sample_count )
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
	default: FT_ASSERT( false ); return static_cast<u32>( -1 );
	}
}

static inline MTLStorageMode
to_mtl_storage_mode( MemoryUsage usage )
{
	// TODO:
	switch ( usage )
	{
	case MemoryUsage::CPU_TO_GPU: return MTLStorageModeShared;
	case MemoryUsage::GPU_ONLY: return MTLStorageModeManaged;
	default: return MTLStorageModeManaged;
	}
}

static inline MTLTextureUsage
to_mtl_texture_usage( DescriptorType type )
{
	MTLTextureUsage usage = 0;
	if ( ( b32 ) ( type & DescriptorType::SAMPLED_IMAGE ) )
		usage |= MTLTextureUsageShaderRead;
	if ( ( b32 ) ( type & DescriptorType::STORAGE_IMAGE ) )
		usage |= MTLTextureUsageShaderWrite;
	if ( ( b32 ) ( type & DescriptorType::COLOR_ATTACHMENT ) )
		usage |= MTLTextureUsageRenderTarget;
	if ( ( b32 ) ( type & DescriptorType::INPUT_ATTACHMENT ) )
		usage |= MTLTextureUsageRenderTarget;
	if ( ( b32 ) ( type & DescriptorType::DEPTH_STENCIL_ATTACHMENT ) )
		usage |= MTLTextureUsageRenderTarget;
	return usage;
}

static inline MTLVertexFormat
to_mtl_vertex_format( Format format )
{
	switch ( format )
	{
	case Format::R8G8_UNORM: return MTLVertexFormatUChar2Normalized;
	case Format::R8G8B8_UNORM: return MTLVertexFormatUChar3Normalized;
	case Format::R8G8B8A8_UNORM: return MTLVertexFormatUChar4Normalized;

	case Format::R8G8_SNORM: return MTLVertexFormatChar2Normalized;
	case Format::R8G8B8_SNORM: return MTLVertexFormatChar3Normalized;
	case Format::R8G8B8A8_SNORM: return MTLVertexFormatChar4Normalized;

	case Format::R16G16_UNORM: return MTLVertexFormatUShort2Normalized;
	case Format::R16G16B16_UNORM: return MTLVertexFormatUShort3Normalized;
	case Format::R16G16B16A16_UNORM: return MTLVertexFormatUShort4Normalized;

	case Format::R16G16_SNORM: return MTLVertexFormatShort2Normalized;
	case Format::R16G16B16_SNORM: return MTLVertexFormatShort3Normalized;
	case Format::R16G16B16A16_SNORM: return MTLVertexFormatShort4Normalized;

	case Format::R16G16_SINT: return MTLVertexFormatShort2;
	case Format::R16G16B16_SINT: return MTLVertexFormatShort3;
	case Format::R16G16B16A16_SINT: return MTLVertexFormatShort4;

	case Format::R16G16_UINT: return MTLVertexFormatUShort2;
	case Format::R16G16B16_UINT: return MTLVertexFormatUShort3;
	case Format::R16G16B16A16_UINT: return MTLVertexFormatUShort4;

	case Format::R16G16_SFLOAT: return MTLVertexFormatHalf2;
	case Format::R16G16B16_SFLOAT: return MTLVertexFormatHalf3;
	case Format::R16G16B16A16_SFLOAT: return MTLVertexFormatHalf4;

	case Format::R32_SFLOAT: return MTLVertexFormatFloat;
	case Format::R32G32_SFLOAT: return MTLVertexFormatFloat2;
	case Format::R32G32B32_SFLOAT: return MTLVertexFormatFloat3;
	case Format::R32G32B32A32_SFLOAT: return MTLVertexFormatFloat4;

	case Format::R32_SINT: return MTLVertexFormatInt;
	case Format::R32G32_SINT: return MTLVertexFormatInt2;
	case Format::R32G32B32_SINT: return MTLVertexFormatInt3;
	case Format::R32G32B32A32_SINT: return MTLVertexFormatInt4;

	case Format::R32_UINT: return MTLVertexFormatUInt;
	case Format::R32G32_UINT: return MTLVertexFormatUInt2;
	case Format::R32G32B32_UINT: return MTLVertexFormatUInt3;
	case Format::R32G32B32A32_UINT: return MTLVertexFormatUInt4;
	default: FT_ASSERT( false ); return MTLVertexFormatInvalid;
	}
}

static inline MTLVertexStepFunction
to_mtl_vertex_step_function( VertexInputRate input_rate )
{
	switch ( input_rate )
	{
	case VertexInputRate::VERTEX: return MTLVertexStepFunctionPerVertex;
	case VertexInputRate::INSTANCE: return MTLVertexStepFunctionPerInstance;
	default: FT_ASSERT( false ); return MTLVertexStepFunction( -1 );
	}
}

static inline MTLSamplerMinMagFilter
to_mtl_sampler_min_mag_filter( Filter filter )
{
	switch ( filter )
	{
	case Filter::LINEAR: return MTLSamplerMinMagFilterLinear;
	case Filter::NEAREST: return MTLSamplerMinMagFilterNearest;
	default: FT_ASSERT( false ); return MTLSamplerMinMagFilter( -1 );
	}
}

static inline MTLSamplerMipFilter
to_mtl_sampler_mip_filter( SamplerMipmapMode mode )
{
	switch ( mode )
	{
	case SamplerMipmapMode::LINEAR: return MTLSamplerMipFilterLinear;
	case SamplerMipmapMode::NEAREST: return MTLSamplerMipFilterNearest;
	default: FT_ASSERT( false ); return MTLSamplerMipFilter( -1 );
	}
}

static inline MTLSamplerAddressMode
to_mtl_sampler_address_mode( SamplerAddressMode mode )
{
	switch ( mode )
	{
	case SamplerAddressMode::REPEAT: return MTLSamplerAddressModeRepeat;
	case SamplerAddressMode::MIRRORED_REPEAT:
		return MTLSamplerAddressModeMirrorRepeat;
	case SamplerAddressMode::CLAMP_TO_EDGE:
		return MTLSamplerAddressModeClampToEdge;
	case SamplerAddressMode::CLAMP_TO_BORDER:
		return MTLSamplerAddressModeClampToEdge;
	default: FT_ASSERT( false ); return MTLSamplerAddressMode( -1 );
	}
}

static inline MTLCompareFunction
to_mtl_compare_function( CompareOp op )
{
	switch ( op )
	{
	case CompareOp::NEVER: return MTLCompareFunctionNever;
	case CompareOp::LESS: return MTLCompareFunctionLess;
	case CompareOp::EQUAL: return MTLCompareFunctionEqual;
	case CompareOp::LESS_OR_EQUAL: return MTLCompareFunctionLessEqual;
	case CompareOp::GREATER: return MTLCompareFunctionGreater;
	case CompareOp::NOT_EQUAL: return MTLCompareFunctionNotEqual;
	case CompareOp::GREATER_OR_EQUAL: return MTLCompareFunctionGreaterEqual;
	case CompareOp::ALWAYS: return MTLCompareFunctionAlways;
	default: FT_ASSERT( false ); return MTLCompareFunction( -1 );
	}
}

static inline MTLPrimitiveType
to_mtl_primitive_type( PrimitiveTopology topology )
{
	switch ( topology )
	{
	case PrimitiveTopology::POINT_LIST: return MTLPrimitiveTypePoint;
	case PrimitiveTopology::LINE_LIST: return MTLPrimitiveTypeLine;
	case PrimitiveTopology::LINE_STRIP: return MTLPrimitiveTypeLineStrip;
	case PrimitiveTopology::TRIANGLE_LIST: return MTLPrimitiveTypeTriangle;
	case PrimitiveTopology::TRIANGLE_STRIP:
		return MTLPrimitiveTypeTriangleStrip;
	default: FT_ASSERT( false ); return MTLPrimitiveType( -1 );
	}
}

void
mtl_destroy_renderer_backend( RendererBackend* ibackend )
{
	FT_ASSERT( ibackend );
	FT_FROM_HANDLE( backend, ibackend, MetalRendererBackend );
	std::free( backend );
}

void
mtl_create_device( const RendererBackend* ibackend,
                   const DeviceInfo*      info,
                   Device**               p )
{
	@autoreleasepool
	{
		FT_ASSERT( p );

		FT_INIT_INTERNAL( device, *p, MetalDevice );

		auto* window   = static_cast<SDL_Window*>( get_app_window()->handle );
		device->device = MTLCreateSystemDefaultDevice();
		device->view   = SDL_Metal_CreateView( window );
	}
}

void
mtl_destroy_device( Device* idevice )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );

		FT_FROM_HANDLE( device, idevice, MetalDevice );

		SDL_Metal_DestroyView( device->view );
		device->device = nil;
		std::free( device );
	}
}

void
mtl_create_queue( const Device* idevice, const QueueInfo* info, Queue** p )
{
	@autoreleasepool
	{
		FT_ASSERT( p );

		FT_FROM_HANDLE( device, idevice, MetalDevice );

		FT_INIT_INTERNAL( queue, *p, MetalQueue );

		queue->queue = [device->device newCommandQueue];
	}
}

void
mtl_destroy_queue( Queue* iqueue )
{
	@autoreleasepool
	{
		FT_ASSERT( iqueue );

		FT_FROM_HANDLE( queue, iqueue, MetalQueue );

		queue->queue = nil;
		std::free( queue );
	}
}

void
mtl_queue_wait_idle( const Queue* iqueue )
{
	@autoreleasepool
	{
		FT_ASSERT( iqueue );

		FT_FROM_HANDLE( queue, iqueue, MetalQueue );

		id<MTLCommandBuffer> wait_cmd =
		    [queue->queue commandBufferWithUnretainedReferences];

		[wait_cmd commit];
		[wait_cmd waitUntilCompleted];
		wait_cmd = nil;
	}
}

void
mtl_queue_submit( const Queue* iqueue, const QueueSubmitInfo* info )
{
}

void
mtl_immediate_submit( const Queue* iqueue, CommandBuffer* cmd )
{
	QueueSubmitInfo queue_submit_info {};
	queue_submit_info.command_buffer_count = 1;
	queue_submit_info.command_buffers      = &cmd;
	queue_submit( iqueue, &queue_submit_info );
	queue_wait_idle( iqueue );
}

void
mtl_queue_present( const Queue* iqueue, const QueuePresentInfo* info )
{
	@autoreleasepool
	{
		FT_FROM_HANDLE( queue, iqueue, MetalQueue );
		FT_FROM_HANDLE( swapchain, info->swapchain, MetalSwapchain );
		FT_FROM_HANDLE(
		    image,
		    swapchain->interface.images[ swapchain->current_image_index ],
		    MetalImage );

		id<MTLCommandBuffer> present_cmd = [queue->queue commandBuffer];
		[present_cmd presentDrawable:swapchain->drawable];
		[present_cmd commit];
		present_cmd         = nil;
		swapchain->drawable = nil;
		image->texture      = nil;
	}
}

void
mtl_create_semaphore( const Device* idevice, Semaphore** p )
{
	FT_ASSERT( p );

	FT_INIT_INTERNAL( semaphore, *p, MetalSemaphore );
}

void
mtl_destroy_semaphore( const Device* idevice, Semaphore* isemaphore )
{
	FT_ASSERT( isemaphore );
	FT_FROM_HANDLE( semaphore, isemaphore, MetalSemaphore );
	std::free( semaphore );
}

void
mtl_create_fence( const Device* idevice, Fence** p )
{
	FT_ASSERT( idevice );
	FT_ASSERT( p );

	FT_INIT_INTERNAL( fence, *p, MetalFence );
}

void
mtl_destroy_fence( const Device* idevice, Fence* ifence )
{
	FT_ASSERT( idevice );
	FT_ASSERT( ifence );
	FT_FROM_HANDLE( fence, ifence, MetalFence );
	std::free( fence );
}

void
mtl_wait_for_fences( const Device* idevice, u32 count, Fence** ifences )
{
}

void
mtl_reset_fences( const Device* idevice, u32 count, Fence** ifences )
{
}

void
mtl_create_swapchain( const Device*        idevice,
                      const SwapchainInfo* info,
                      Swapchain**          p )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( info );
		FT_ASSERT( p );

		FT_FROM_HANDLE( device, idevice, MetalDevice );

		FT_INIT_INTERNAL( swapchain, *p, MetalSwapchain );

		swapchain->interface.min_image_count = info->min_image_count;
		swapchain->interface.image_count     = info->min_image_count;
		swapchain->interface.width           = info->width;
		swapchain->interface.height          = info->height;
		swapchain->interface.format          = info->format;
		swapchain->interface.queue           = info->queue;
		swapchain->interface.vsync           = info->vsync;

		swapchain->swapchain =
		    ( __bridge CAMetalLayer* ) SDL_Metal_GetLayer( device->view );
		swapchain->swapchain.device             = device->device;
		swapchain->swapchain.pixelFormat        = to_mtl_format( info->format );
		swapchain->swapchain.displaySyncEnabled = info->vsync;

		swapchain->interface.image_count =
		    swapchain->swapchain.maximumDrawableCount;

		swapchain->interface.images = static_cast<Image**>(
		    std::calloc( swapchain->interface.image_count, sizeof( Image* ) ) );

		for ( u32 i = 0; i < swapchain->interface.image_count; i++ )
		{
			FT_INIT_INTERNAL( image,
			                  swapchain->interface.images[ i ],
			                  MetalImage );

			image->texture                   = nil;
			image->interface.width           = info->width;
			image->interface.height          = info->height;
			image->interface.depth           = 1;
			image->interface.format          = info->format;
			image->interface.layer_count     = 1;
			image->interface.descriptor_type = DescriptorType::COLOR_ATTACHMENT;
			image->interface.mip_level_count = 1;
			image->interface.sample_count    = SampleCount::E1;
		}
	}
}

void
mtl_resize_swapchain( const Device* idevice,
                      Swapchain*    iswapchain,
                      u32           width,
                      u32           height )
{
	FT_ASSERT( idevice );
	FT_ASSERT( iswapchain );

	FT_FROM_HANDLE( swapchain, iswapchain, MetalSwapchain );

	swapchain->interface.width  = width;
	swapchain->interface.height = height;
}

void
mtl_destroy_swapchain( const Device* idevice, Swapchain* iswapchain )
{
	FT_ASSERT( iswapchain );

	FT_FROM_HANDLE( swapchain, iswapchain, MetalSwapchain );

	for ( u32 i = 0; i < swapchain->interface.image_count; i++ )
	{
		FT_FROM_HANDLE( image, swapchain->interface.images[ i ], MetalImage );
		std::free( image );
	}

	std::free( swapchain->interface.images );
	std::free( swapchain );
}

void
mtl_create_command_pool( const Device*          idevice,
                         const CommandPoolInfo* info,
                         CommandPool**          p )
{
	FT_ASSERT( idevice );
	FT_ASSERT( info );
	FT_ASSERT( p );

	FT_INIT_INTERNAL( cmd_pool, *p, MetalCommandPool );

	cmd_pool->interface.queue = info->queue;
}

void
mtl_destroy_command_pool( const Device* idevice, CommandPool* icommand_pool )
{
	FT_ASSERT( idevice );
	FT_ASSERT( icommand_pool );

	FT_FROM_HANDLE( cmd_pool, icommand_pool, MetalCommandPool );

	std::free( cmd_pool );
}

void
mtl_create_command_buffers( const Device*      idevice,
                            const CommandPool* icommand_pool,
                            u32                count,
                            CommandBuffer**    p )
{
	FT_ASSERT( idevice );
	FT_ASSERT( icommand_pool );
	FT_ASSERT( p );

	FT_FROM_HANDLE( cmd_pool, icommand_pool, MetalCommandPool );

	for ( u32 i = 0; i < count; i++ )
	{
		FT_INIT_INTERNAL( cmd, p[ i ], MetalCommandBuffer );
		cmd->interface.queue = cmd_pool->interface.queue;
	}
}

void
mtl_free_command_buffers( const Device*      idevice,
                          const CommandPool* icommand_pool,
                          u32                count,
                          CommandBuffer**    icommand_buffers )
{
}

void
mtl_destroy_command_buffers( const Device*      idevice,
                             const CommandPool* icmd_pool,
                             u32                count,
                             CommandBuffer**    icommand_buffers )
{
	FT_ASSERT( idevice );
	FT_ASSERT( icmd_pool );

	for ( u32 i = 0; i < count; i++ )
	{
		FT_FROM_HANDLE( cmd, icommand_buffers[ i ], MetalCommandBuffer );
		std::free( cmd );
	}
}

void
mtl_begin_command_buffer( const CommandBuffer* icmd )
{
	@autoreleasepool
	{
		FT_ASSERT( icmd );
		FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
		FT_FROM_HANDLE( queue, cmd->interface.queue, MetalQueue );

		cmd->cmd = [queue->queue commandBuffer];
	}
}

void
mtl_end_command_buffer( const CommandBuffer* icmd )
{
	@autoreleasepool
	{
		FT_ASSERT( icmd );
		FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
		[cmd->cmd commit];
		cmd->cmd = nil;
	}
}

void
mtl_acquire_next_image( const Device*    idevice,
                        const Swapchain* iswapchain,
                        const Semaphore* isemaphore,
                        const Fence*     ifence,
                        u32*             image_index )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( iswapchain );

		FT_FROM_HANDLE( swapchain, iswapchain, MetalSwapchain );
		FT_FROM_HANDLE(
		    image,
		    swapchain->interface.images[ swapchain->current_image_index ],
		    MetalImage );

		swapchain->drawable = [swapchain->swapchain nextDrawable];
		image->texture      = swapchain->drawable.texture;

		*image_index = swapchain->current_image_index;
		swapchain->current_image_index =
		    ( swapchain->current_image_index + 1 ) %
		    swapchain->interface.image_count;
	}
}

void
mtl_create_shader( const Device* idevice, ShaderInfo* info, Shader** p )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( info );
		FT_ASSERT( p );

		FT_FROM_HANDLE( device, idevice, MetalDevice );

		FT_INIT_INTERNAL( shader, *p, MetalShader );

		new ( &shader->interface.reflect_data.binding_map ) BindingMap();
		new ( &shader->interface.reflect_data.bindings ) Bindings();

		auto create_function = []( const MetalDevice*      device,
		                           MetalShader*            shader,
		                           ShaderStage             stage,
		                           const ShaderModuleInfo& info )
		{
			dispatch_data_t lib_data =
			    dispatch_data_create( info.bytecode,
			                          info.bytecode_size,
			                          nil,
			                          DISPATCH_DATA_DESTRUCTOR_DEFAULT );
			id<MTLLibrary> library = [device->device newLibraryWithData:lib_data
			                                                      error:nil];

			shader->shaders[ static_cast<u32>( stage ) ] =
			    [library newFunctionWithName:@"main0"];
		};

		create_function( device, shader, ShaderStage::COMPUTE, info->compute );
		create_function( device, shader, ShaderStage::VERTEX, info->vertex );
		create_function( device,
		                 shader,
		                 ShaderStage::TESSELLATION_CONTROL,
		                 info->tessellation_control );
		create_function( device,
		                 shader,
		                 ShaderStage::TESSELLATION_EVALUATION,
		                 info->tessellation_evaluation );
		create_function( device,
		                 shader,
		                 ShaderStage::FRAGMENT,
		                 info->fragment );

		mtl_reflect( idevice, info, &shader->interface );
	}
}

void
mtl_destroy_shader( const Device* idevice, Shader* ishader )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( ishader );

		FT_FROM_HANDLE( shader, ishader, MetalShader );

		for ( u32 i = 0; i < static_cast<u32>( ShaderStage::COUNT ); ++i )
		{
			if ( shader->shaders[ i ] )
			{
				shader->shaders[ i ] = nil;
			}
		}

		shader->interface.reflect_data.binding_map.~unordered_map();
		shader->interface.reflect_data.bindings.~vector();
		std::free( shader );
	}
}

void
mtl_create_descriptor_set_layout( const Device*         idevice,
                                  Shader*               ishader,
                                  DescriptorSetLayout** p )
{
	FT_INIT_INTERNAL( layout, *p, MetalDescriptorSetLayout );

	new ( &layout->interface.reflection_data.bindings ) Bindings();
	new ( &layout->interface.reflection_data.binding_map ) BindingMap();

	layout->interface.reflection_data = ishader->reflect_data;
}

void
mtl_destroy_descriptor_set_layout( const Device*        idevice,
                                   DescriptorSetLayout* ilayout )
{
	FT_FROM_HANDLE( layout, ilayout, MetalDescriptorSetLayout );

	layout->interface.reflection_data.bindings.~vector();
	layout->interface.reflection_data.binding_map.~unordered_map();
	std::free( layout );
}

void
mtl_create_compute_pipeline( const Device*       idevice,
                             const PipelineInfo* info,
                             Pipeline**          p )
{
}

void
mtl_create_graphics_pipeline( const Device*       idevice,
                              const PipelineInfo* info,
                              Pipeline**          p )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( info );
		FT_ASSERT( p );

		FT_FROM_HANDLE( device, idevice, MetalDevice );
		FT_FROM_HANDLE( shader, info->shader, MetalShader );

		FT_INIT_INTERNAL( pipeline, *p, MetalPipeline );

		pipeline->pipeline_descriptor =
		    [[MTLRenderPipelineDescriptor alloc] init];

		for ( u32 i = 0; i < static_cast<u32>( ShaderStage::COUNT ); ++i )
		{
			if ( shader->shaders[ i ] == nil )
			{
				continue;
			}

			switch ( static_cast<ShaderStage>( i ) )
			{
			case ShaderStage::VERTEX:
			{
				pipeline->pipeline_descriptor.vertexFunction =
				    shader->shaders[ i ];
				break;
			}
			case ShaderStage::FRAGMENT:
			{
				pipeline->pipeline_descriptor.fragmentFunction =
				    shader->shaders[ i ];
				break;
			}
			default:
			{
				break;
			}
			}
		}

		MTLVertexDescriptor* vertex_layout =
		    [MTLVertexDescriptor vertexDescriptor];

		for ( u32 i = 0; i < info->vertex_layout.binding_info_count; ++i )
		{
			// TODO: rewrite more elegant? binding should not conflict with
			// uniform bindings
			u32 binding = info->vertex_layout.binding_infos[ i ].binding +
			              MAX_VERTEX_BINDING_COUNT;
			vertex_layout.layouts[ binding ].stepFunction =
			    to_mtl_vertex_step_function(
			        info->vertex_layout.binding_infos[ i ].input_rate );
			vertex_layout.layouts[ binding ].stride =
			    info->vertex_layout.binding_infos[ i ].stride;
		}

		for ( u32 i = 0; i < info->vertex_layout.attribute_info_count; ++i )
		{
			u32 location = info->vertex_layout.attribute_infos[ i ].location;
			vertex_layout.attributes[ location ].format = to_mtl_vertex_format(
			    info->vertex_layout.attribute_infos[ i ].format );
			vertex_layout.attributes[ location ].offset =
			    info->vertex_layout.attribute_infos[ i ].offset;
			vertex_layout.attributes[ location ].bufferIndex =
			    info->vertex_layout.attribute_infos[ i ].binding +
			    MAX_VERTEX_BINDING_COUNT;
		}

		pipeline->pipeline_descriptor.vertexDescriptor = vertex_layout;

		for ( u32 i = 0; i < info->color_attachment_count; ++i )
		{
			pipeline->pipeline_descriptor.colorAttachments[ i ].pixelFormat =
			    to_mtl_format( info->color_attachment_formats[ i ] );
		}

		if ( info->depth_stencil_format != Format::UNDEFINED )
		{
			pipeline->pipeline_descriptor.depthAttachmentPixelFormat =
			    to_mtl_format( info->depth_stencil_format );
		}

		if ( info->depth_state_info.depth_test )
		{
			MTLDepthStencilDescriptor* depth_stencil_descriptor =
			    [[MTLDepthStencilDescriptor alloc] init];
			depth_stencil_descriptor.depthCompareFunction =
			    to_mtl_compare_function( info->depth_state_info.compare_op );
			depth_stencil_descriptor.depthWriteEnabled =
			    info->depth_state_info.depth_write;
			pipeline->depth_stencil_state = [device->device
			    newDepthStencilStateWithDescriptor:depth_stencil_descriptor];
		}

		pipeline->pipeline = [device->device
		    newRenderPipelineStateWithDescriptor:pipeline->pipeline_descriptor
		                                   error:nil];

		pipeline->primitive_type = to_mtl_primitive_type( info->topology );
	}
}

void
mtl_destroy_pipeline( const Device* idevice, Pipeline* ipipeline )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( ipipeline );

		FT_FROM_HANDLE( pipeline, ipipeline, MetalPipeline );

		if ( pipeline->depth_stencil_state != nil )
		{
			pipeline->depth_stencil_state = nil;
		}

		pipeline->pipeline            = nil;
		pipeline->pipeline_descriptor = nil;
		std::free( pipeline );
	}
}

void
mtl_create_buffer( const Device* idevice, const BufferInfo* info, Buffer** p )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( info );
		FT_ASSERT( p );

		FT_FROM_HANDLE( device, idevice, MetalDevice );

		FT_INIT_INTERNAL( buffer, *p, MetalBuffer );

		buffer->interface.size            = info->size;
		buffer->interface.descriptor_type = info->descriptor_type;
		buffer->interface.memory_usage    = info->memory_usage;
		buffer->interface.resource_state  = ResourceState::UNDEFINED;

		buffer->buffer = [device->device
		    newBufferWithLength:info->size
		                options:MTLResourceCPUCacheModeDefaultCache];
	}
}

void
mtl_destroy_buffer( const Device* idevice, Buffer* ibuffer )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( ibuffer );

		FT_FROM_HANDLE( buffer, ibuffer, MetalBuffer );
		buffer->buffer = nil;
		std::free( buffer );
	}
}

void*
mtl_map_memory( const Device* idevice, Buffer* ibuffer )
{
	@autoreleasepool
	{
		FT_FROM_HANDLE( buffer, ibuffer, MetalBuffer );
		buffer->interface.mapped_memory = buffer->buffer.contents;
		return buffer->interface.mapped_memory;
	}
}

void
mtl_unmap_memory( const Device* idevice, Buffer* ibuffer )
{
	@autoreleasepool
	{
		ibuffer->mapped_memory = nullptr;
	}
}

void
mtl_create_sampler( const Device*      idevice,
                    const SamplerInfo* info,
                    Sampler**          p )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( info );
		FT_ASSERT( p );

		FT_FROM_HANDLE( device, idevice, MetalDevice );

		FT_INIT_INTERNAL( sampler, *p, MetalSampler );

		MTLSamplerDescriptor* sampler_descriptor =
		    [[MTLSamplerDescriptor alloc] init];
		sampler_descriptor.minFilter =
		    to_mtl_sampler_min_mag_filter( info->min_filter );
		sampler_descriptor.magFilter =
		    to_mtl_sampler_min_mag_filter( info->mag_filter );
		sampler_descriptor.mipFilter =
		    to_mtl_sampler_mip_filter( info->mipmap_mode );
		sampler_descriptor.sAddressMode =
		    to_mtl_sampler_address_mode( info->address_mode_u );
		sampler_descriptor.tAddressMode =
		    to_mtl_sampler_address_mode( info->address_mode_v );
		sampler_descriptor.rAddressMode =
		    to_mtl_sampler_address_mode( info->address_mode_w );
		sampler_descriptor.maxAnisotropy = info->max_anisotropy;
		sampler_descriptor.compareFunction =
		    to_mtl_compare_function( info->compare_op );
		sampler_descriptor.lodMinClamp = info->min_lod;
		sampler_descriptor.lodMaxClamp = info->max_lod;

		sampler->sampler =
		    [device->device newSamplerStateWithDescriptor:sampler_descriptor];
	}
}

void
mtl_destroy_sampler( const Device* idevice, Sampler* isampler )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( isampler );

		FT_FROM_HANDLE( sampler, isampler, MetalSampler );

		std::free( sampler );
	}
}

void
mtl_create_image( const Device* idevice, const ImageInfo* info, Image** p )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( info );
		FT_ASSERT( p );

		FT_FROM_HANDLE( device, idevice, MetalDevice );

		FT_INIT_INTERNAL( image, *p, MetalImage );

		image->interface.width           = info->width;
		image->interface.height          = info->height;
		image->interface.depth           = info->depth;
		image->interface.format          = info->format;
		image->interface.descriptor_type = info->descriptor_type;
		image->interface.sample_count    = info->sample_count;
		image->interface.layer_count     = info->layer_count;
		image->interface.mip_level_count = info->mip_levels;

		MTLTextureDescriptor* texture_descriptor =
		    [[MTLTextureDescriptor alloc] init];

		texture_descriptor.width            = info->width;
		texture_descriptor.height           = info->height;
		texture_descriptor.depth            = info->depth;
		texture_descriptor.arrayLength      = info->layer_count;
		texture_descriptor.mipmapLevelCount = info->mip_levels;
		texture_descriptor.sampleCount =
		    to_mtl_sample_count( info->sample_count );
		texture_descriptor.storageMode = MTLStorageModeManaged; // TODO:
		texture_descriptor.textureType = MTLTextureType2D;      // TODO:
		texture_descriptor.usage =
		    to_mtl_texture_usage( info->descriptor_type );
		texture_descriptor.pixelFormat = to_mtl_format( info->format );

		image->texture =
		    [device->device newTextureWithDescriptor:texture_descriptor];

		texture_descriptor = nil;
	}
}

void
mtl_destroy_image( const Device* idevice, Image* iimage )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( iimage );

		FT_FROM_HANDLE( image, iimage, MetalImage );

		image->texture = nil;
		std::free( image );
	}
}

static inline void
count_binding_types( const ReflectionData*             reflection,
                     std::vector<MetalSamplerBinding>& sampler_bindings,
                     std::vector<MetalImageBinding>&   image_bindings,
                     std::vector<MetalBufferBinding>&  buffer_bindings )
{
	u32   binding_count = reflection->binding_count;
	auto& bindings      = reflection->bindings;

	for ( u32 b = 0; b < binding_count; ++b )
	{
		auto& binding = bindings[ b ];
		switch ( binding.descriptor_type )
		{
		case DescriptorType::SAMPLER:
		{
			auto& b   = sampler_bindings.emplace_back();
			b         = {};
			b.stage   = binding.stage;
			b.binding = binding.binding;
			break;
		}
		case DescriptorType::SAMPLED_IMAGE:
		{
			auto& b   = image_bindings.emplace_back();
			b         = {};
			b.stage   = binding.stage;
			b.binding = binding.binding;

			break;
		}
		case DescriptorType::UNIFORM_BUFFER:
		{
			auto& b   = buffer_bindings.emplace_back();
			b         = {};
			b.stage   = binding.stage;
			b.binding = binding.binding;
			break;
		}
		default:
		{
			break;
		}
		}
	}
}

void
mtl_create_descriptor_set( const Device*            idevice,
                           const DescriptorSetInfo* info,
                           DescriptorSet**          p )
{
	FT_ASSERT( idevice );
	FT_ASSERT( info );
	FT_ASSERT( p );

	FT_INIT_INTERNAL( set, *p, MetalDescriptorSet );

	set->interface.layout = info->descriptor_set_layout;

	std::vector<MetalSamplerBinding> sampler_bindings;
	std::vector<MetalImageBinding>   image_bindings;
	std::vector<MetalBufferBinding>  buffer_bindings;

	count_binding_types( &set->interface.layout->reflection_data,
	                     sampler_bindings,
	                     image_bindings,
	                     buffer_bindings );

	set->sampler_binding_count = sampler_bindings.size();
	set->buffer_binding_count  = buffer_bindings.size();
	set->image_binding_count   = image_bindings.size();

	set->sampler_bindings = nil;
	if ( set->sampler_binding_count > 0 )
	{
		set->sampler_bindings = static_cast<MetalSamplerBinding*>(
		    std::calloc( set->sampler_binding_count,
		                 sizeof( MetalSamplerBinding ) ) );

		std::copy( sampler_bindings.begin(),
		           sampler_bindings.end(),
		           set->sampler_bindings );
	}

	set->image_bindings = nil;
	if ( set->image_binding_count > 0 )
	{
		set->image_bindings = static_cast<MetalImageBinding*>(
		    std::calloc( set->image_binding_count,
		                 sizeof( MetalImageBinding ) ) );

		std::copy( image_bindings.begin(),
		           image_bindings.end(),
		           set->image_bindings );
	}

	set->buffer_bindings = nil;
	if ( set->buffer_binding_count > 0 )
	{
		set->buffer_bindings = static_cast<MetalBufferBinding*>(
		    std::calloc( set->buffer_binding_count,
		                 sizeof( MetalBufferBinding ) ) );

		std::copy( buffer_bindings.begin(),
		           buffer_bindings.end(),
		           set->buffer_bindings );
	}
}

void
mtl_destroy_descriptor_set( const Device* idevice, DescriptorSet* iset )
{
	FT_ASSERT( idevice );
	FT_ASSERT( iset );

	FT_FROM_HANDLE( set, iset, MetalDescriptorSet );

	if ( set->sampler_bindings )
	{
		std::free( set->sampler_bindings );
	}

	if ( set->image_bindings )
	{
		std::free( set->image_bindings );
	}

	if ( set->buffer_bindings )
	{
		std::free( set->buffer_bindings );
	}

	std::free( set );
}

void
mtl_update_descriptor_set( const Device*          idevice,
                           DescriptorSet*         iset,
                           u32                    count,
                           const DescriptorWrite* writes )
{
	@autoreleasepool
	{
		FT_FROM_HANDLE( set, iset, MetalDescriptorSet );

		const auto& binding_map =
		    set->interface.layout->reflection_data.binding_map;
		const auto& bindings = set->interface.layout->reflection_data.bindings;

		for ( u32 i = 0; i < count; ++i )
		{
			FT_ASSERT( binding_map.find( writes[ i ].descriptor_name ) !=
			           binding_map.cend() );

			const Binding* binding =
			    &bindings[ binding_map.at( writes[ i ].descriptor_name ) ];

			switch ( binding->descriptor_type )
			{
			case DescriptorType::SAMPLER:
			{
				for ( u32 j = 0; j < set->sampler_binding_count; ++j )
				{
					if ( set->sampler_bindings[ j ].binding ==
					     binding->binding )
					{
						FT_FROM_HANDLE(
						    sampler,
						    writes[ i ].sampler_descriptors[ 0 ].sampler,
						    MetalSampler );

						set->sampler_bindings[ j ].sampler = sampler->sampler;
					}
				}
				break;
			}
			case DescriptorType::SAMPLED_IMAGE:
			{
				for ( u32 j = 0; j < set->image_binding_count; ++j )
				{
					if ( set->image_bindings[ j ].binding == binding->binding )
					{
						FT_FROM_HANDLE(
						    image,
						    writes[ i ].image_descriptors[ 0 ].image,
						    MetalImage );

						set->image_bindings[ j ].image = image->texture;
					}
				}
				break;
			}
			case DescriptorType::UNIFORM_BUFFER:
			{
				for ( u32 j = 0; j < set->buffer_binding_count; ++j )
				{
					if ( set->buffer_bindings[ i ].binding == binding->binding )
					{
						FT_FROM_HANDLE(
						    buffer,
						    writes[ i ].buffer_descriptors[ 0 ].buffer,
						    MetalBuffer );

						set->buffer_bindings[ j ].buffer = buffer->buffer;
					}
				}
				break;
			}
			default:
			{
				break;
			}
			}
		}
	}
}

void
mtl_init_ui( const UiInfo* info )
{
	@autoreleasepool
	{
		FT_FROM_HANDLE( device, info->device, MetalDevice );

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

		ImGui_ImplSDL2_InitForMetal(
		    static_cast<SDL_Window*>( info->window->handle ) );
		ImGui_ImplMetal_Init( device->device );
	}
}

void
mtl_shutdown_ui( const Device* idevice )
{
	@autoreleasepool
	{
		ImGui_ImplMetal_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}
}

void
mtl_ui_upload_resources( CommandBuffer* cmd )
{
}

void
mtl_ui_destroy_upload_objects()
{
}

void
mtl_ui_begin_frame( CommandBuffer* icmd )
{
	@autoreleasepool
	{
		FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
		ImGui_ImplMetal_NewFrame( cmd->pass_descriptor );
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
	}
}

void
mtl_ui_end_frame( CommandBuffer* icmd )
{
	@autoreleasepool
	{
		FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
		ImGui::Render();
		FT_ASSERT( cmd->encoder != nil );
		ImGui_ImplMetal_RenderDrawData( ImGui::GetDrawData(),
		                                cmd->cmd,
		                                cmd->encoder );

		ImGuiIO& io = ImGui::GetIO();
		if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}
}

void*
mtl_get_imgui_texture_id( const Image* iimage )
{
	@autoreleasepool
	{
		FT_FROM_HANDLE( image, iimage, MetalImage );
		return ( __bridge void* ) image->texture;
	}
}

void
mtl_cmd_begin_render_pass( const CommandBuffer*       icmd,
                           const RenderPassBeginInfo* info )
{
	@autoreleasepool
	{
		FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );

		// TODO: fix
		MTLRenderPassDescriptor* pass_descriptor =
		    [MTLRenderPassDescriptor renderPassDescriptor];

		for ( u32 i = 0; i < info->color_attachment_count; ++i )
		{
			FT_FROM_HANDLE( image, info->color_attachments[ i ], MetalImage );
			pass_descriptor.colorAttachments[ i ].loadAction =
			    to_mtl_load_action( info->color_attachment_load_ops[ i ] );
			pass_descriptor.colorAttachments[ i ].storeAction =
			    MTLStoreActionStore;

			pass_descriptor.colorAttachments[ i ].texture = image->texture;

			pass_descriptor.colorAttachments[ i ].clearColor =
			    MTLClearColorMake( info->clear_values[ i ].color[ 0 ],
			                       info->clear_values[ i ].color[ 1 ],
			                       info->clear_values[ i ].color[ 2 ],
			                       info->clear_values[ i ].color[ 3 ] );
			// TODO:
			pass_descriptor.renderTargetWidth  = image->interface.width;
			pass_descriptor.renderTargetHeight = image->interface.height;
			pass_descriptor.defaultRasterSampleCount =
			    to_mtl_sample_count( image->interface.sample_count );
		}

		if ( info->depth_stencil != nullptr )
		{
			FT_FROM_HANDLE( image, info->depth_stencil, MetalImage );
			pass_descriptor.depthAttachment.texture = image->texture;
			pass_descriptor.depthAttachment.loadAction =
			    to_mtl_load_action( info->depth_stencil_load_op );
			pass_descriptor.depthAttachment.storeAction = MTLStoreActionStore;
			pass_descriptor.depthAttachment.texture     = image->texture;

			pass_descriptor.depthAttachment.clearDepth =
			    info->clear_values[ info->color_attachment_count ]
			        .depth_stencil.depth;

			// TODO:
			pass_descriptor.renderTargetWidth  = image->interface.width;
			pass_descriptor.renderTargetHeight = image->interface.height;
			pass_descriptor.defaultRasterSampleCount =
			    to_mtl_sample_count( image->interface.sample_count );
		}

		cmd->encoder =
		    [cmd->cmd renderCommandEncoderWithDescriptor:pass_descriptor];
		cmd->pass_descriptor = pass_descriptor;
	}
}

void
mtl_cmd_end_render_pass( const CommandBuffer* icmd )
{
	@autoreleasepool
	{
		FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );

		[cmd->encoder endEncoding];
		cmd->encoder = nil;
	}
}

void
mtl_cmd_barrier( const CommandBuffer* icmd,
                 u32,
                 const MemoryBarrier*,
                 u32                  buffer_barriers_count,
                 const BufferBarrier* buffer_barriers,
                 u32                  image_barriers_count,
                 const ImageBarrier*  image_barriers ) {};

void
mtl_cmd_set_scissor( const CommandBuffer* icmd,
                     i32                  x,
                     i32                  y,
                     u32                  width,
                     u32                  height )
{
	FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );

	[cmd->encoder setScissorRect:( MTLScissorRect ) { static_cast<u32>( x ),
	                                                  static_cast<u32>( y ),
	                                                  width,
	                                                  height }];
}

void
mtl_cmd_set_viewport( const CommandBuffer* icmd,
                      f32                  x,
                      f32                  y,
                      f32                  width,
                      f32                  height,
                      f32                  min_depth,
                      f32                  max_depth )
{
	FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );

	[cmd->encoder setViewport:( MTLViewport ) { x,
	                                            y,
	                                            width,
	                                            height,
	                                            min_depth,
	                                            max_depth }];
}

void
mtl_cmd_bind_pipeline( const CommandBuffer* icmd, const Pipeline* ipipeline )
{
	FT_ASSERT( icmd );
	FT_ASSERT( ipipeline );

	FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
	FT_FROM_HANDLE( pipeline, ipipeline, MetalPipeline );

	if ( pipeline->depth_stencil_state )
	{
		[cmd->encoder setDepthStencilState:pipeline->depth_stencil_state];
	}
	[cmd->encoder setRenderPipelineState:pipeline->pipeline];

	cmd->primitive_type = pipeline->primitive_type;
}

void
mtl_cmd_draw( const CommandBuffer* icmd,
              u32                  vertex_count,
              u32                  instance_count,
              u32                  first_vertex,
              u32                  first_instance )
{
	FT_ASSERT( icmd );

	FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );

	[cmd->encoder drawPrimitives:cmd->primitive_type
	                 vertexStart:first_vertex
	                 vertexCount:vertex_count];
}

void
mtl_cmd_draw_indexed( const CommandBuffer* icmd,
                      u32                  index_count,
                      u32                  instance_count,
                      u32                  first_index,
                      i32                  vertex_offset,
                      u32                  first_instance )
{
	FT_ASSERT( icmd );

	FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );

	[cmd->encoder drawIndexedPrimitives:cmd->primitive_type
	                         indexCount:index_count
	                          indexType:cmd->index_type
	                        indexBuffer:cmd->index_buffer
	                  indexBufferOffset:first_index
	                      instanceCount:1
	                         baseVertex:vertex_offset
	                       baseInstance:first_instance];
}

void
mtl_cmd_bind_vertex_buffer( const CommandBuffer* icmd,
                            const Buffer*        ibuffer,
                            const u64            offset )
{
	FT_ASSERT( icmd );
	FT_ASSERT( ibuffer );

	FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
	FT_FROM_HANDLE( buffer, ibuffer, MetalBuffer );

	[cmd->encoder setVertexBuffer:buffer->buffer
	                       offset:offset
	                      atIndex:MAX_VERTEX_BINDING_COUNT];
}

void
mtl_cmd_bind_index_buffer_u16( const CommandBuffer* icmd,
                               const Buffer*        ibuffer,
                               const u64            offset )
{
	@autoreleasepool
	{
		FT_ASSERT( icmd );
		FT_ASSERT( ibuffer );

		FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
		FT_FROM_HANDLE( buffer, ibuffer, MetalBuffer );

		cmd->index_type   = MTLIndexTypeUInt16;
		cmd->index_buffer = buffer->buffer;
	}
}

void
mtl_cmd_bind_index_buffer_u32( const CommandBuffer* icmd,
                               const Buffer*        ibuffer,
                               u64                  offset )
{
	@autoreleasepool
	{
		FT_ASSERT( icmd );
		FT_ASSERT( ibuffer );

		FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
		FT_FROM_HANDLE( buffer, ibuffer, MetalBuffer );

		cmd->index_type   = MTLIndexTypeUInt32;
		cmd->index_buffer = buffer->buffer;
	}
}

void
mtl_cmd_copy_buffer( const CommandBuffer* icmd,
                     const Buffer*        isrc,
                     u64                  src_offset,
                     Buffer*              idst,
                     u64                  dst_offset,
                     u64                  size )
{
	FT_FROM_HANDLE( src, isrc, MetalBuffer );
	FT_FROM_HANDLE( dst, idst, MetalBuffer );

	u8* src_ptr = static_cast<u8*>( src->buffer.contents );
	u8* dst_ptr = static_cast<u8*>( dst->buffer.contents );

	std::memcpy( dst_ptr + dst_offset, src_ptr + src_offset, size );
}

void
mtl_cmd_copy_buffer_to_image( const CommandBuffer* icmd,
                              const Buffer*        isrc,
                              u64                  src_offset,
                              Image*               idst )
{
	FT_FROM_HANDLE( src, isrc, MetalBuffer );
	FT_FROM_HANDLE( dst, idst, MetalImage );

	u8* src_ptr = static_cast<u8*>( src->buffer.contents );

	MTLRegion region = {
		{ 0, 0, 0 },
		{ dst->interface.width, dst->interface.height, dst->interface.depth }
	};

	NSUInteger bytesPerRow = 4 * dst->interface.width;
	[dst->texture replaceRegion:region
	                mipmapLevel:0
	                  withBytes:src_ptr + src_offset
	                bytesPerRow:bytesPerRow];
}

void
mtl_cmd_dispatch( const CommandBuffer* icmd,
                  u32                  group_count_x,
                  u32                  group_count_y,
                  u32                  group_count_z )
{
}

void
mtl_cmd_push_constants( const CommandBuffer* icmd,
                        const Pipeline*      ipipeline,
                        u64                  offset,
                        u64                  size,
                        const void*          data )
{
}

void
mtl_cmd_blit_image( const CommandBuffer* icmd,
                    const Image*         isrc,
                    ResourceState        src_state,
                    Image*               idst,
                    ResourceState        dst_state,
                    Filter               filter )
{
}

void
mtl_cmd_clear_color_image( const CommandBuffer* icmd,
                           Image*               iimage,
                           Vector4              color )
{
}

void
mtl_cmd_draw_indexed_indirect( const CommandBuffer* icmd,
                               const Buffer*        ibuffer,
                               u64                  offset,
                               u32                  draw_count,
                               u32                  stride )
{
}

void
mtl_cmd_bind_descriptor_set( const CommandBuffer* icmd,
                             u32                  first_set,
                             const DescriptorSet* iset,
                             const Pipeline*      ipipeline )
{
	FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
	FT_FROM_HANDLE( set, iset, MetalDescriptorSet );

	for ( u32 i = 0; i < set->sampler_binding_count; ++i )
	{
		switch ( set->sampler_bindings[ i ].stage )
		{
		case ShaderStage::VERTEX:
		{
			[cmd->encoder
			    setVertexSamplerState:set->sampler_bindings[ i ].sampler
			                  atIndex:set->sampler_bindings[ i ].binding];
			break;
		}
		case ShaderStage::FRAGMENT:
		{
			[cmd->encoder
			    setFragmentSamplerState:set->sampler_bindings[ i ].sampler
			                    atIndex:set->sampler_bindings[ i ].binding];
			break;
		}
		default:
		{
			break;
		}
		}
	}

	for ( u32 i = 0; i < set->image_binding_count; ++i )
	{
		switch ( set->image_bindings[ i ].stage )
		{
		case ShaderStage::VERTEX:
		{
			[cmd->encoder setVertexTexture:set->image_bindings[ i ].image
			                       atIndex:set->image_bindings[ i ].binding];
			break;
		}
		case ShaderStage::FRAGMENT:
		{
			[cmd->encoder setFragmentTexture:set->image_bindings[ i ].image
			                         atIndex:set->image_bindings[ i ].binding];
			break;
		}
		default:
		{
			break;
		}
		}
	}

	for ( u32 i = 0; i < set->buffer_binding_count; ++i )
	{
		switch ( set->buffer_bindings[ i ].stage )
		{
		case ShaderStage::VERTEX:
		{
			[cmd->encoder setVertexBuffer:set->buffer_bindings[ i ].buffer
			                       offset:0
			                      atIndex:set->buffer_bindings[ i ].binding];
			break;
		}
		case ShaderStage::FRAGMENT:
		{
			[cmd->encoder setFragmentBuffer:set->buffer_bindings[ i ].buffer
			                         offset:0
			                        atIndex:set->buffer_bindings[ i ].binding];
			break;
		}
		default:
		{
			break;
		}
		}
	}
}

std::vector<char>
mtl_read_shader( const std::string& shader_name )
{
	return fs::read_file_binary( fs::get_shaders_directory() + "metal/" +
	                             shader_name + ".bin" );
}

void
mtl_create_renderer_backend( const RendererBackendInfo*, RendererBackend** p )
{
	FT_ASSERT( p );

	destroy_renderer_backend      = mtl_destroy_renderer_backend;
	create_device                 = mtl_create_device;
	destroy_device                = mtl_destroy_device;
	create_queue                  = mtl_create_queue;
	destroy_queue                 = mtl_destroy_queue;
	queue_wait_idle               = mtl_queue_wait_idle;
	queue_submit                  = mtl_queue_submit;
	immediate_submit              = mtl_immediate_submit;
	queue_present                 = mtl_queue_present;
	create_semaphore              = mtl_create_semaphore;
	destroy_semaphore             = mtl_destroy_semaphore;
	create_fence                  = mtl_create_fence;
	destroy_fence                 = mtl_destroy_fence;
	wait_for_fences               = mtl_wait_for_fences;
	reset_fences                  = mtl_reset_fences;
	create_swapchain              = mtl_create_swapchain;
	resize_swapchain              = mtl_resize_swapchain;
	destroy_swapchain             = mtl_destroy_swapchain;
	create_command_pool           = mtl_create_command_pool;
	destroy_command_pool          = mtl_destroy_command_pool;
	create_command_buffers        = mtl_create_command_buffers;
	free_command_buffers          = mtl_free_command_buffers;
	destroy_command_buffers       = mtl_destroy_command_buffers;
	begin_command_buffer          = mtl_begin_command_buffer;
	end_command_buffer            = mtl_end_command_buffer;
	acquire_next_image            = mtl_acquire_next_image;
	create_shader                 = mtl_create_shader;
	destroy_shader                = mtl_destroy_shader;
	create_descriptor_set_layout  = mtl_create_descriptor_set_layout;
	destroy_descriptor_set_layout = mtl_destroy_descriptor_set_layout;
	create_compute_pipeline       = mtl_create_compute_pipeline;
	create_graphics_pipeline      = mtl_create_graphics_pipeline;
	destroy_pipeline              = mtl_destroy_pipeline;
	create_buffer                 = mtl_create_buffer;
	destroy_buffer                = mtl_destroy_buffer;
	map_memory                    = mtl_map_memory;
	unmap_memory                  = mtl_unmap_memory;
	create_sampler                = mtl_create_sampler;
	destroy_sampler               = mtl_destroy_sampler;
	create_image                  = mtl_create_image;
	destroy_image                 = mtl_destroy_image;
	create_descriptor_set         = mtl_create_descriptor_set;
	destroy_descriptor_set        = mtl_destroy_descriptor_set;
	update_descriptor_set         = mtl_update_descriptor_set;
	init_ui                       = mtl_init_ui;
	shutdown_ui                   = mtl_shutdown_ui;
	ui_upload_resources           = mtl_ui_upload_resources;
	ui_destroy_upload_objects     = mtl_ui_destroy_upload_objects;
	ui_begin_frame                = mtl_ui_begin_frame;
	ui_end_frame                  = mtl_ui_end_frame;
	get_imgui_texture_id          = mtl_get_imgui_texture_id;
	cmd_begin_render_pass         = mtl_cmd_begin_render_pass;
	cmd_end_render_pass           = mtl_cmd_end_render_pass;
	cmd_barrier                   = mtl_cmd_barrier;
	cmd_set_scissor               = mtl_cmd_set_scissor;
	cmd_set_viewport              = mtl_cmd_set_viewport;
	cmd_bind_pipeline             = mtl_cmd_bind_pipeline;
	cmd_draw                      = mtl_cmd_draw;
	cmd_draw_indexed              = mtl_cmd_draw_indexed;
	cmd_bind_vertex_buffer        = mtl_cmd_bind_vertex_buffer;
	cmd_bind_index_buffer_u16     = mtl_cmd_bind_index_buffer_u16;
	cmd_bind_index_buffer_u32     = mtl_cmd_bind_index_buffer_u32;
	cmd_copy_buffer               = mtl_cmd_copy_buffer;
	cmd_copy_buffer_to_image      = mtl_cmd_copy_buffer_to_image;
	cmd_bind_descriptor_set       = mtl_cmd_bind_descriptor_set;
	cmd_dispatch                  = mtl_cmd_dispatch;
	cmd_push_constants            = mtl_cmd_push_constants;
	cmd_blit_image                = mtl_cmd_blit_image;
	cmd_clear_color_image         = mtl_cmd_clear_color_image;
	cmd_draw_indexed_indirect     = mtl_cmd_draw_indexed_indirect;

	read_shader = mtl_read_shader;

	FT_INIT_INTERNAL( render_backend, *p, MetalRendererBackend );
}

}
#endif
