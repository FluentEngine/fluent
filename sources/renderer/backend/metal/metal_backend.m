#ifdef METAL_BACKEND

#ifndef METAL_BACKEND_INCLUDE_OBJC
#define METAL_BACKEND_INCLUDE_OBJC
#endif
#include <SDL.h>
#include <tiny_image_format/tinyimageformat_apis.h>
#include "wsi/wsi.h"
#include "os/window.h"
#include "../renderer_private.h"
#include "metal_backend.h"

static inline MTLPixelFormat
to_mtl_format( enum Format format )
{
	return ( MTLPixelFormat ) TinyImageFormat_ToMTLPixelFormat(
	    ( TinyImageFormat ) format );
}

static inline MTLLoadAction
to_mtl_load_action( enum AttachmentLoadOp load_op )
{
	switch ( load_op )
	{
	case FT_ATTACHMENT_LOAD_OP_LOAD: return MTLLoadActionLoad;
	case FT_ATTACHMENT_LOAD_OP_CLEAR: return MTLLoadActionClear;
	case FT_ATTACHMENT_LOAD_OP_DONT_CARE: return MTLLoadActionDontCare;
	default: FT_ASSERT( 0 ); return ( MTLLoadAction ) -1;
	}
}

static inline MTLStorageMode
to_mtl_storage_mode( enum MemoryUsage usage )
{
	// TODO:
	switch ( usage )
	{
	case FT_MEMORY_USAGE_CPU_TO_GPU: return MTLStorageModeShared;
	case FT_MEMORY_USAGE_GPU_ONLY: return MTLStorageModeManaged;
	default: return MTLStorageModeManaged;
	}
}

static inline MTLTextureUsage
to_mtl_texture_usage( enum DescriptorType type )
{
	MTLTextureUsage usage = 0;
	if ( ( b32 ) ( type & FT_DESCRIPTOR_TYPE_SAMPLED_IMAGE ) )
		usage |= MTLTextureUsageShaderRead;
	if ( ( b32 ) ( type & FT_DESCRIPTOR_TYPE_STORAGE_IMAGE ) )
		usage |= MTLTextureUsageShaderWrite;
	if ( ( b32 ) ( type & FT_DESCRIPTOR_TYPE_COLOR_ATTACHMENT ) )
		usage |= MTLTextureUsageRenderTarget;
	if ( ( b32 ) ( type & FT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT ) )
		usage |= MTLTextureUsageRenderTarget;
	if ( ( b32 ) ( type & FT_DESCRIPTOR_TYPE_DEPTH_STENCIL_ATTACHMENT ) )
		usage |= MTLTextureUsageRenderTarget;
	return usage;
}

static inline MTLVertexFormat
to_mtl_vertex_format( enum Format format )
{
	switch ( format )
	{
	case FT_FORMAT_R8G8_UNORM: return MTLVertexFormatUChar2Normalized;
	case FT_FORMAT_R8G8B8_UNORM: return MTLVertexFormatUChar3Normalized;
	case FT_FORMAT_R8G8B8A8_UNORM: return MTLVertexFormatUChar4Normalized;

	case FT_FORMAT_R8G8_SNORM: return MTLVertexFormatChar2Normalized;
	case FT_FORMAT_R8G8B8_SNORM: return MTLVertexFormatChar3Normalized;
	case FT_FORMAT_R8G8B8A8_SNORM: return MTLVertexFormatChar4Normalized;

	case FT_FORMAT_R16G16_UNORM: return MTLVertexFormatUShort2Normalized;
	case FT_FORMAT_R16G16B16_UNORM: return MTLVertexFormatUShort3Normalized;
	case FT_FORMAT_R16G16B16A16_UNORM: return MTLVertexFormatUShort4Normalized;

	case FT_FORMAT_R16G16_SNORM: return MTLVertexFormatShort2Normalized;
	case FT_FORMAT_R16G16B16_SNORM: return MTLVertexFormatShort3Normalized;
	case FT_FORMAT_R16G16B16A16_SNORM: return MTLVertexFormatShort4Normalized;

	case FT_FORMAT_R16G16_SINT: return MTLVertexFormatShort2;
	case FT_FORMAT_R16G16B16_SINT: return MTLVertexFormatShort3;
	case FT_FORMAT_R16G16B16A16_SINT: return MTLVertexFormatShort4;

	case FT_FORMAT_R16G16_UINT: return MTLVertexFormatUShort2;
	case FT_FORMAT_R16G16B16_UINT: return MTLVertexFormatUShort3;
	case FT_FORMAT_R16G16B16A16_UINT: return MTLVertexFormatUShort4;

	case FT_FORMAT_R16G16_SFLOAT: return MTLVertexFormatHalf2;
	case FT_FORMAT_R16G16B16_SFLOAT: return MTLVertexFormatHalf3;
	case FT_FORMAT_R16G16B16A16_SFLOAT: return MTLVertexFormatHalf4;

	case FT_FORMAT_R32_SFLOAT: return MTLVertexFormatFloat;
	case FT_FORMAT_R32G32_SFLOAT: return MTLVertexFormatFloat2;
	case FT_FORMAT_R32G32B32_SFLOAT: return MTLVertexFormatFloat3;
	case FT_FORMAT_R32G32B32A32_SFLOAT: return MTLVertexFormatFloat4;

	case FT_FORMAT_R32_SINT: return MTLVertexFormatInt;
	case FT_FORMAT_R32G32_SINT: return MTLVertexFormatInt2;
	case FT_FORMAT_R32G32B32_SINT: return MTLVertexFormatInt3;
	case FT_FORMAT_R32G32B32A32_SINT: return MTLVertexFormatInt4;

	case FT_FORMAT_R32_UINT: return MTLVertexFormatUInt;
	case FT_FORMAT_R32G32_UINT: return MTLVertexFormatUInt2;
	case FT_FORMAT_R32G32B32_UINT: return MTLVertexFormatUInt3;
	case FT_FORMAT_R32G32B32A32_UINT: return MTLVertexFormatUInt4;
	default: FT_ASSERT( 0 ); return MTLVertexFormatInvalid;
	}
}

static inline MTLVertexStepFunction
to_mtl_vertex_step_function( enum VertexInputRate input_rate )
{
	switch ( input_rate )
	{
	case FT_VERTEX_INPUT_RATE_VERTEX: return MTLVertexStepFunctionPerVertex;
	case FT_VERTEX_INPUT_RATE_INSTANCE: return MTLVertexStepFunctionPerInstance;
	default: FT_ASSERT( 0 ); return ( MTLVertexStepFunction ) -1;
	}
}

static inline MTLSamplerMinMagFilter
to_mtl_sampler_min_mag_filter( enum Filter filter )
{
	switch ( filter )
	{
	case FT_FILTER_LINEAR: return MTLSamplerMinMagFilterLinear;
	case FT_FILTER_NEAREST: return MTLSamplerMinMagFilterNearest;
	default: FT_ASSERT( 0 ); return ( MTLSamplerMinMagFilter ) -1;
	}
}

static inline MTLSamplerMipFilter
to_mtl_sampler_mip_filter( enum SamplerMipmapMode mode )
{
	switch ( mode )
	{
	case FT_SAMPLER_MIPMAP_MODE_LINEAR: return MTLSamplerMipFilterLinear;
	case FT_SAMPLER_MIPMAP_MODE_NEAREST: return MTLSamplerMipFilterNearest;
	default: FT_ASSERT( 0 ); return ( MTLSamplerMipFilter ) -1;
	}
}

static inline MTLSamplerAddressMode
to_mtl_sampler_address_mode( enum SamplerAddressMode mode )
{
	switch ( mode )
	{
	case FT_SAMPLER_ADDRESS_MODE_REPEAT: return MTLSamplerAddressModeRepeat;
	case FT_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
		return MTLSamplerAddressModeMirrorRepeat;
	case FT_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
		return MTLSamplerAddressModeClampToEdge;
	case FT_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
		return MTLSamplerAddressModeClampToEdge;
	default: FT_ASSERT( 0 ); return ( MTLSamplerAddressMode ) -1;
	}
}

static inline MTLCompareFunction
to_mtl_compare_function( enum CompareOp op )
{
	switch ( op )
	{
	case FT_COMPARE_OP_NEVER: return MTLCompareFunctionNever;
	case FT_COMPARE_OP_LESS: return MTLCompareFunctionLess;
	case FT_COMPARE_OP_EQUAL: return MTLCompareFunctionEqual;
	case FT_COMPARE_OP_LESS_OR_EQUAL: return MTLCompareFunctionLessEqual;
	case FT_COMPARE_OP_GREATER: return MTLCompareFunctionGreater;
	case FT_COMPARE_OP_NOT_EQUAL: return MTLCompareFunctionNotEqual;
	case FT_COMPARE_OP_GREATER_OR_EQUAL: return MTLCompareFunctionGreaterEqual;
	case FT_COMPARE_OP_ALWAYS: return MTLCompareFunctionAlways;
	default: FT_ASSERT( 0 ); return ( MTLCompareFunction ) -1;
	}
}

static inline MTLPrimitiveType
to_mtl_primitive_type( enum PrimitiveTopology topology )
{
	switch ( topology )
	{
	case FT_PRIMITIVE_TOPOLOGY_POINT_LIST: return MTLPrimitiveTypePoint;
	case FT_PRIMITIVE_TOPOLOGY_LINE_LIST: return MTLPrimitiveTypeLine;
	case FT_PRIMITIVE_TOPOLOGY_LINE_STRIP: return MTLPrimitiveTypeLineStrip;
	case FT_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: return MTLPrimitiveTypeTriangle;
	case FT_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
		return MTLPrimitiveTypeTriangleStrip;
	default: FT_ASSERT( 0 ); return ( MTLPrimitiveType ) -1;
	}
}

static void
mtl_destroy_renderer_backend( struct RendererBackend* ibackend )
{
	FT_ASSERT( ibackend );
	FT_FROM_HANDLE( backend, ibackend, MetalRendererBackend );
	free( backend );
}

static void
mtl_create_device( const struct RendererBackend* ibackend,
                   const struct DeviceInfo*      info,
                   struct Device**               p )
{
	@autoreleasepool
	{
		FT_ASSERT( p );

		FT_FROM_HANDLE( backend, ibackend, MetalRendererBackend );

		FT_INIT_INTERNAL( device, *p, MetalDevice );

		// TODO: different wsi support
		SDL_Window* window = backend->window;
		device->device     = MTLCreateSystemDefaultDevice();
		FT_ASSERT( device->device && "Failed to create mtl device" );
		device->view = SDL_Metal_CreateView( window );
		FT_ASSERT( device->view && "failed to create mtl view" );
	}
}

static void
mtl_destroy_device( struct Device* idevice )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );

		FT_FROM_HANDLE( device, idevice, MetalDevice );

		SDL_Metal_DestroyView( device->view );
		device->device = nil;
		free( device );
	}
}

static void
mtl_create_queue( const struct Device*    idevice,
                  const struct QueueInfo* info,
                  struct Queue**          p )
{
	@autoreleasepool
	{
		FT_ASSERT( p );

		FT_FROM_HANDLE( device, idevice, MetalDevice );

		FT_INIT_INTERNAL( queue, *p, MetalQueue );

		queue->queue = [device->device newCommandQueue];
	}
}

static void
mtl_destroy_queue( struct Queue* iqueue )
{
	@autoreleasepool
	{
		FT_ASSERT( iqueue );

		FT_FROM_HANDLE( queue, iqueue, MetalQueue );

		queue->queue = nil;
		free( queue );
	}
}

static void
mtl_queue_wait_idle( const struct Queue* iqueue )
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

static void
mtl_queue_submit( const struct Queue*           iqueue,
                  const struct QueueSubmitInfo* info )
{
}

static void
mtl_immediate_submit( const struct Queue* iqueue, struct CommandBuffer* cmd )
{
	struct QueueSubmitInfo queue_submit_info = { 0 };
	queue_submit_info.command_buffer_count   = 1;
	queue_submit_info.command_buffers        = &cmd;
	queue_submit( iqueue, &queue_submit_info );
	queue_wait_idle( iqueue );
}

static void
mtl_queue_present( const struct Queue*            iqueue,
                   const struct QueuePresentInfo* info )
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

static void
mtl_create_semaphore( const struct Device* idevice, struct Semaphore** p )
{
	FT_ASSERT( p );

	FT_INIT_INTERNAL( semaphore, *p, MetalSemaphore );
}

static void
mtl_destroy_semaphore( const struct Device* idevice,
                       struct Semaphore*    isemaphore )
{
	FT_ASSERT( isemaphore );
	FT_FROM_HANDLE( semaphore, isemaphore, MetalSemaphore );
	free( semaphore );
}

static void
mtl_create_fence( const struct Device* idevice, struct Fence** p )
{
	FT_ASSERT( idevice );
	FT_ASSERT( p );

	FT_INIT_INTERNAL( fence, *p, MetalFence );
}

static void
mtl_destroy_fence( const struct Device* idevice, struct Fence* ifence )
{
	FT_ASSERT( idevice );
	FT_ASSERT( ifence );
	FT_FROM_HANDLE( fence, ifence, MetalFence );
	free( fence );
}

static void
mtl_wait_for_fences( const struct Device* idevice,
                     u32                  count,
                     struct Fence**       ifences )
{
}

static void
mtl_reset_fences( const struct Device* idevice,
                  u32                  count,
                  struct Fence**       ifences )
{
}

static void
mtl_create_swapchain( const struct Device*        idevice,
                      const struct SwapchainInfo* info,
                      struct Swapchain**          p )
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

		swapchain->interface.images =
		    calloc( swapchain->interface.image_count, sizeof( struct Image* ) );

		for ( u32 i = 0; i < swapchain->interface.image_count; i++ )
		{
			FT_INIT_INTERNAL( image,
			                  swapchain->interface.images[ i ],
			                  MetalImage );

			image->texture               = nil;
			image->interface.width       = info->width;
			image->interface.height      = info->height;
			image->interface.depth       = 1;
			image->interface.format      = info->format;
			image->interface.layer_count = 1;
			image->interface.descriptor_type =
			    FT_DESCRIPTOR_TYPE_COLOR_ATTACHMENT;
			image->interface.mip_level_count = 1;
			image->interface.sample_count    = 1;
		}
	}
}

static void
mtl_resize_swapchain( const struct Device* idevice,
                      struct Swapchain*    iswapchain,
                      u32                  width,
                      u32                  height )
{
	FT_ASSERT( idevice );
	FT_ASSERT( iswapchain );

	FT_FROM_HANDLE( swapchain, iswapchain, MetalSwapchain );

	swapchain->interface.width  = width;
	swapchain->interface.height = height;
}

static void
mtl_destroy_swapchain( const struct Device* idevice,
                       struct Swapchain*    iswapchain )
{
	FT_ASSERT( iswapchain );

	FT_FROM_HANDLE( swapchain, iswapchain, MetalSwapchain );

	for ( u32 i = 0; i < swapchain->interface.image_count; i++ )
	{
		FT_FROM_HANDLE( image, swapchain->interface.images[ i ], MetalImage );
		free( image );
	}

	free( swapchain->interface.images );
	free( swapchain );
}

static void
mtl_create_command_pool( const struct Device*          idevice,
                         const struct CommandPoolInfo* info,
                         struct CommandPool**          p )
{
	FT_ASSERT( idevice );
	FT_ASSERT( info );
	FT_ASSERT( p );

	FT_INIT_INTERNAL( cmd_pool, *p, MetalCommandPool );

	cmd_pool->interface.queue = info->queue;
}

static void
mtl_destroy_command_pool( const struct Device* idevice,
                          struct CommandPool*  icommand_pool )
{
	FT_ASSERT( idevice );
	FT_ASSERT( icommand_pool );

	FT_FROM_HANDLE( cmd_pool, icommand_pool, MetalCommandPool );

	free( cmd_pool );
}

static void
mtl_create_command_buffers( const struct Device*      idevice,
                            const struct CommandPool* icommand_pool,
                            u32                       count,
                            struct CommandBuffer**    p )
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

static void
mtl_free_command_buffers( const struct Device*      idevice,
                          const struct CommandPool* icommand_pool,
                          u32                       count,
                          struct CommandBuffer**    icommand_buffers )
{
}

static void
mtl_destroy_command_buffers( const struct Device*      idevice,
                             const struct CommandPool* icmd_pool,
                             u32                       count,
                             struct CommandBuffer**    icommand_buffers )
{
	FT_ASSERT( idevice );
	FT_ASSERT( icmd_pool );

	for ( u32 i = 0; i < count; i++ )
	{
		FT_FROM_HANDLE( cmd, icommand_buffers[ i ], MetalCommandBuffer );
		free( cmd );
	}
}

static void
mtl_begin_command_buffer( const struct CommandBuffer* icmd )
{
	@autoreleasepool
	{
		FT_ASSERT( icmd );
		FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
		FT_FROM_HANDLE( queue, cmd->interface.queue, MetalQueue );

		cmd->cmd = [queue->queue commandBuffer];
	}
}

static void
mtl_end_command_buffer( const struct CommandBuffer* icmd )
{
	@autoreleasepool
	{
		FT_ASSERT( icmd );
		FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
		[cmd->cmd commit];
		cmd->cmd = nil;
	}
}

static void
mtl_acquire_next_image( const struct Device*    idevice,
                        const struct Swapchain* iswapchain,
                        const struct Semaphore* isemaphore,
                        const struct Fence*     ifence,
                        u32*                    image_index )
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

static void
mtl_create_function( const struct MetalDevice*      device,
                     struct MetalShader*            shader,
                     enum ShaderStage               stage,
                     const struct ShaderModuleInfo* info )
{
	if ( info->bytecode )
	{
		dispatch_data_t lib_data =
		    dispatch_data_create( info->bytecode,
		                          info->bytecode_size,
		                          nil,
		                          DISPATCH_DATA_DESTRUCTOR_DEFAULT );
		id<MTLLibrary> library = [device->device newLibraryWithData:lib_data
		                                                      error:nil];

		shader->shaders[ stage ] = [library newFunctionWithName:@"main0"];
	}
}

static void
mtl_create_shader( const struct Device* idevice,
                   struct ShaderInfo*   info,
                   struct Shader**      p )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( info );
		FT_ASSERT( p );

		FT_FROM_HANDLE( device, idevice, MetalDevice );

		FT_INIT_INTERNAL( shader, *p, MetalShader );

		mtl_create_function( device,
		                     shader,
		                     FT_SHADER_STAGE_COMPUTE,
		                     &info->compute );
		mtl_create_function( device,
		                     shader,
		                     FT_SHADER_STAGE_VERTEX,
		                     &info->vertex );
		mtl_create_function( device,
		                     shader,
		                     FT_SHADER_STAGE_TESSELLATION_CONTROL,
		                     &info->tessellation_control );
		mtl_create_function( device,
		                     shader,
		                     FT_SHADER_STAGE_TESSELLATION_EVALUATION,
		                     &info->tessellation_evaluation );
		mtl_create_function( device,
		                     shader,
		                     FT_SHADER_STAGE_FRAGMENT,
		                     &info->fragment );

		mtl_reflect( idevice, info, &shader->interface );
	}
}

static void
mtl_destroy_shader( const struct Device* idevice, struct Shader* ishader )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( ishader );

		FT_FROM_HANDLE( shader, ishader, MetalShader );

		for ( u32 i = 0; i < FT_SHADER_STAGE_COUNT; ++i )
		{
			if ( shader->shaders[ i ] )
			{
				shader->shaders[ i ] = nil;
			}
		}

		free( shader );
	}
}

static void
mtl_create_descriptor_set_layout( const struct Device*         idevice,
                                  struct Shader*               ishader,
                                  struct DescriptorSetLayout** p )
{
	FT_INIT_INTERNAL( layout, *p, MetalDescriptorSetLayout );

	layout->interface.reflection_data = ishader->reflect_data;
}

static void
mtl_destroy_descriptor_set_layout( const struct Device*        idevice,
                                   struct DescriptorSetLayout* ilayout )
{
	FT_FROM_HANDLE( layout, ilayout, MetalDescriptorSetLayout );

	free( layout );
}

static void
mtl_create_compute_pipeline( const struct Device*       idevice,
                             const struct PipelineInfo* info,
                             struct Pipeline**          p )
{
}

static void
mtl_create_graphics_pipeline( const struct Device*       idevice,
                              const struct PipelineInfo* info,
                              struct Pipeline**          p )
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

		for ( u32 i = 0; i < FT_SHADER_STAGE_COUNT; ++i )
		{
			if ( shader->shaders[ i ] == nil )
			{
				continue;
			}

			switch ( i )
			{
			case FT_SHADER_STAGE_VERTEX:
			{
				pipeline->pipeline_descriptor.vertexFunction =
				    shader->shaders[ i ];
				break;
			}
			case FT_SHADER_STAGE_FRAGMENT:
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

		if ( info->depth_stencil_format != FT_FORMAT_UNDEFINED )
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

static void
mtl_destroy_pipeline( const struct Device* idevice, struct Pipeline* ipipeline )
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
		free( pipeline );
	}
}

static void
mtl_create_buffer( const struct Device*     idevice,
                   const struct BufferInfo* info,
                   struct Buffer**          p )
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
		buffer->interface.resource_state  = FT_RESOURCE_STATE_UNDEFINED;

		buffer->buffer = [device->device
		    newBufferWithLength:info->size
		                options:MTLResourceCPUCacheModeDefaultCache];
	}
}

static void
mtl_destroy_buffer( const struct Device* idevice, struct Buffer* ibuffer )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( ibuffer );

		FT_FROM_HANDLE( buffer, ibuffer, MetalBuffer );
		buffer->buffer = nil;
		free( buffer );
	}
}

static void*
mtl_map_memory( const struct Device* idevice, struct Buffer* ibuffer )
{
	@autoreleasepool
	{
		FT_FROM_HANDLE( buffer, ibuffer, MetalBuffer );
		buffer->interface.mapped_memory = buffer->buffer.contents;
		return buffer->interface.mapped_memory;
	}
}

static void
mtl_unmap_memory( const struct Device* idevice, struct Buffer* ibuffer )
{
	ibuffer->mapped_memory = NULL;
}

static void
mtl_create_sampler( const struct Device*      idevice,
                    const struct SamplerInfo* info,
                    struct Sampler**          p )
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

static void
mtl_destroy_sampler( const struct Device* idevice, struct Sampler* isampler )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( isampler );

		FT_FROM_HANDLE( sampler, isampler, MetalSampler );

		free( sampler );
	}
}

static void
mtl_create_image( const struct Device*    idevice,
                  const struct ImageInfo* info,
                  struct Image**          p )
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
		texture_descriptor.sampleCount      = info->sample_count;
		texture_descriptor.storageMode      = MTLStorageModeManaged; // TODO:
		texture_descriptor.textureType      = MTLTextureType2D;      // TODO:
		texture_descriptor.usage =
		    to_mtl_texture_usage( info->descriptor_type );
		texture_descriptor.pixelFormat = to_mtl_format( info->format );

		image->texture =
		    [device->device newTextureWithDescriptor:texture_descriptor];

		texture_descriptor = nil;
	}
}

static void
mtl_destroy_image( const struct Device* idevice, struct Image* iimage )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( iimage );

		FT_FROM_HANDLE( image, iimage, MetalImage );

		image->texture = nil;
		free( image );
	}
}

static inline void
count_binding_types( const ReflectionData*        reflection,
                     u32*                         sampler_binding_count,
                     struct MetalSamplerBinding** sampler_bindings,
                     u32*                         image_binding_count,
                     struct MetalImageBinding**   image_bindings,
                     u32*                         buffer_binding_count,
                     struct MetalBufferBinding**  buffer_bindings )
{
	u32             binding_count = reflection->binding_count;
	struct Binding* bindings      = reflection->bindings;

	u32 s = 0;
	u32 i = 0;
	u32 b = 0;

	sampler_bindings =
	    calloc( binding_count, sizeof( struct MetalSamplerBinding* ) );
	image_bindings =
	    calloc( binding_count, sizeof( struct MetalImageBinding* ) );
	buffer_bindings = calloc( binding_count, sizeof( struct BufferBinding* ) );

	for ( u32 b = 0; b < binding_count; ++b )
	{
		struct Binding* binding = &bindings[ b ];
		switch ( binding->descriptor_type )
		{
		case FT_DESCRIPTOR_TYPE_SAMPLER:
		{
			struct MetalSamplerBinding* sb = sampler_bindings[ s++ ];
			sb->stage                      = binding->stage;
			sb->binding                    = binding->binding;
			break;
		}
		case FT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		{
			struct MetalImageBinding* ib = image_bindings[ i++ ];
			ib->stage                    = binding->stage;
			ib->binding                  = binding->binding;

			break;
		}
		case FT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		{
			struct MetalBufferBinding* bb = buffer_bindings[ b++ ];
			bb->stage                     = binding->stage;
			bb->binding                   = binding->binding;
			break;
		}
		default:
		{
			break;
		}
		}
	}

	*sampler_binding_count = s;
	*buffer_binding_count  = b;
	*image_binding_count   = i;
}

static void
mtl_create_descriptor_set( const struct Device*            idevice,
                           const struct DescriptorSetInfo* info,
                           struct DescriptorSet**          p )
{
	FT_ASSERT( idevice );
	FT_ASSERT( info );
	FT_ASSERT( p );

	FT_INIT_INTERNAL( set, *p, MetalDescriptorSet );

	set->interface.layout = info->descriptor_set_layout;

	count_binding_types( &set->interface.layout->reflection_data,
	                     &set->sampler_binding_count,
	                     &set->sampler_bindings,
	                     &set->image_binding_count,
	                     &set->image_bindings,
	                     &set->buffer_binding_count,
	                     &set->buffer_bindings );
}

static void
mtl_destroy_descriptor_set( const struct Device*  idevice,
                            struct DescriptorSet* iset )
{
	FT_ASSERT( idevice );
	FT_ASSERT( iset );

	FT_FROM_HANDLE( set, iset, MetalDescriptorSet );

	if ( set->sampler_bindings )
	{
		free( set->sampler_bindings );
	}

	if ( set->image_bindings )
	{
		free( set->image_bindings );
	}

	if ( set->buffer_bindings )
	{
		free( set->buffer_bindings );
	}

	free( set );
}

static void
mtl_update_descriptor_set( const struct Device*          idevice,
                           struct DescriptorSet*         iset,
                           u32                           count,
                           const struct DescriptorWrite* writes )
{
#if 0
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
			case FT_DESCRIPTOR_TYPE_SAMPLER:
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
			case FT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
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
			case FT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
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
#endif
}

static void
mtl_cmd_begin_render_pass( const struct CommandBuffer*       icmd,
                           const struct RenderPassBeginInfo* info )
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
			    image->interface.sample_count;
		}

		if ( info->depth_stencil != NULL )
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
			    image->interface.sample_count;
		}

		cmd->encoder =
		    [cmd->cmd renderCommandEncoderWithDescriptor:pass_descriptor];
		cmd->pass_descriptor = pass_descriptor;
	}
}

static void
mtl_cmd_end_render_pass( const struct CommandBuffer* icmd )
{
	@autoreleasepool
	{
		FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );

		[cmd->encoder endEncoding];
		cmd->encoder = nil;
	}
}

static void
mtl_cmd_barrier( const struct CommandBuffer* icmd,
                 u32                         memory_barrier_count,
                 const struct MemoryBarrier* memory_barriers,
                 u32                         buffer_barriers_count,
                 const struct BufferBarrier* buffer_barriers,
                 u32                         image_barriers_count,
                 const struct ImageBarrier*  image_barriers ) {};

static void
mtl_cmd_set_scissor( const struct CommandBuffer* icmd,
                     i32                         x,
                     i32                         y,
                     u32                         width,
                     u32                         height )
{
	FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );

	[cmd->encoder setScissorRect:( MTLScissorRect ) { ( u32 ) x,
	                                                  ( u32 ) y,
	                                                  width,
	                                                  height }];
}

static void
mtl_cmd_set_viewport( const struct CommandBuffer* icmd,
                      f32                         x,
                      f32                         y,
                      f32                         width,
                      f32                         height,
                      f32                         min_depth,
                      f32                         max_depth )
{
	FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );

	[cmd->encoder setViewport:( MTLViewport ) { x,
	                                            y,
	                                            width,
	                                            height,
	                                            min_depth,
	                                            max_depth }];
}

static void
mtl_cmd_bind_pipeline( const struct CommandBuffer* icmd,
                       const struct Pipeline*      ipipeline )
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

static void
mtl_cmd_draw( const struct CommandBuffer* icmd,
              u32                         vertex_count,
              u32                         instance_count,
              u32                         first_vertex,
              u32                         first_instance )
{
	FT_ASSERT( icmd );

	FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );

	[cmd->encoder drawPrimitives:cmd->primitive_type
	                 vertexStart:first_vertex
	                 vertexCount:vertex_count];
}

static void
mtl_cmd_draw_indexed( const struct CommandBuffer* icmd,
                      u32                         index_count,
                      u32                         instance_count,
                      u32                         first_index,
                      i32                         vertex_offset,
                      u32                         first_instance )
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

static void
mtl_cmd_bind_vertex_buffer( const struct CommandBuffer* icmd,
                            const struct Buffer*        ibuffer,
                            const u64                   offset )
{
	FT_ASSERT( icmd );
	FT_ASSERT( ibuffer );

	FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
	FT_FROM_HANDLE( buffer, ibuffer, MetalBuffer );

	[cmd->encoder setVertexBuffer:buffer->buffer
	                       offset:offset
	                      atIndex:MAX_VERTEX_BINDING_COUNT];
}

static void
mtl_cmd_bind_index_buffer_u16( const struct CommandBuffer* icmd,
                               const struct Buffer*        ibuffer,
                               const u64                   offset )
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

static void
mtl_cmd_bind_index_buffer_u32( const struct CommandBuffer* icmd,
                               const struct Buffer*        ibuffer,
                               u64                         offset )
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

static void
mtl_cmd_copy_buffer( const struct CommandBuffer* icmd,
                     const struct Buffer*        isrc,
                     u64                         src_offset,
                     struct Buffer*              idst,
                     u64                         dst_offset,
                     u64                         size )
{
	FT_FROM_HANDLE( src, isrc, MetalBuffer );
	FT_FROM_HANDLE( dst, idst, MetalBuffer );

	u8* src_ptr = src->buffer.contents;
	u8* dst_ptr = dst->buffer.contents;

	memcpy( dst_ptr + dst_offset, src_ptr + src_offset, size );
}

static void
mtl_cmd_copy_buffer_to_image( const struct CommandBuffer* icmd,
                              const struct Buffer*        isrc,
                              u64                         src_offset,
                              struct Image*               idst )
{
	FT_FROM_HANDLE( src, isrc, MetalBuffer );
	FT_FROM_HANDLE( dst, idst, MetalImage );

	u8* src_ptr = src->buffer.contents;

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

static void
mtl_cmd_dispatch( const struct CommandBuffer* icmd,
                  u32                         group_count_x,
                  u32                         group_count_y,
                  u32                         group_count_z )
{
}

static void
mtl_cmd_push_constants( const struct CommandBuffer* icmd,
                        const struct Pipeline*      ipipeline,
                        u32                         offset,
                        u32                         size,
                        const void*                 data )
{
}

static void
mtl_cmd_blit_image( const struct CommandBuffer* icmd,
                    const struct Image*         isrc,
                    enum ResourceState          src_state,
                    struct Image*               idst,
                    enum ResourceState          dst_state,
                    enum Filter                 filter )
{
}

static void
mtl_cmd_clear_color_image( const struct CommandBuffer* icmd,
                           struct Image*               iimage,
                           f32                         color[ 4 ] )
{
}

static void
mtl_cmd_draw_indexed_indirect( const struct CommandBuffer* icmd,
                               const struct Buffer*        ibuffer,
                               u64                         offset,
                               u32                         draw_count,
                               u32                         stride )
{
}

static void
mtl_cmd_bind_descriptor_set( const struct CommandBuffer* icmd,
                             u32                         first_set,
                             const struct DescriptorSet* iset,
                             const struct Pipeline*      ipipeline )
{
	FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
	FT_FROM_HANDLE( set, iset, MetalDescriptorSet );

	for ( u32 i = 0; i < set->sampler_binding_count; ++i )
	{
		switch ( set->sampler_bindings[ i ].stage )
		{
		case FT_SHADER_STAGE_VERTEX:
		{
			[cmd->encoder
			    setVertexSamplerState:set->sampler_bindings[ i ].sampler
			                  atIndex:set->sampler_bindings[ i ].binding];
			break;
		}
		case FT_SHADER_STAGE_FRAGMENT:
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
		case FT_SHADER_STAGE_VERTEX:
		{
			[cmd->encoder setVertexTexture:set->image_bindings[ i ].image
			                       atIndex:set->image_bindings[ i ].binding];
			break;
		}
		case FT_SHADER_STAGE_FRAGMENT:
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
		case FT_SHADER_STAGE_VERTEX:
		{
			[cmd->encoder setVertexBuffer:set->buffer_bindings[ i ].buffer
			                       offset:0
			                      atIndex:set->buffer_bindings[ i ].binding];
			break;
		}
		case FT_SHADER_STAGE_FRAGMENT:
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

void
mtl_create_renderer_backend( const struct RendererBackendInfo* info,
                             struct RendererBackend**          p )
{
	FT_UNUSED( info );
	FT_ASSERT( p );

	destroy_renderer_backend_impl      = mtl_destroy_renderer_backend;
	create_device_impl                 = mtl_create_device;
	destroy_device_impl                = mtl_destroy_device;
	create_queue_impl                  = mtl_create_queue;
	destroy_queue_impl                 = mtl_destroy_queue;
	queue_wait_idle_impl               = mtl_queue_wait_idle;
	queue_submit_impl                  = mtl_queue_submit;
	immediate_submit_impl              = mtl_immediate_submit;
	queue_present_impl                 = mtl_queue_present;
	create_semaphore_impl              = mtl_create_semaphore;
	destroy_semaphore_impl             = mtl_destroy_semaphore;
	create_fence_impl                  = mtl_create_fence;
	destroy_fence_impl                 = mtl_destroy_fence;
	wait_for_fences_impl               = mtl_wait_for_fences;
	reset_fences_impl                  = mtl_reset_fences;
	create_swapchain_impl              = mtl_create_swapchain;
	resize_swapchain_impl              = mtl_resize_swapchain;
	destroy_swapchain_impl             = mtl_destroy_swapchain;
	create_command_pool_impl           = mtl_create_command_pool;
	destroy_command_pool_impl          = mtl_destroy_command_pool;
	create_command_buffers_impl        = mtl_create_command_buffers;
	free_command_buffers_impl          = mtl_free_command_buffers;
	destroy_command_buffers_impl       = mtl_destroy_command_buffers;
	begin_command_buffer_impl          = mtl_begin_command_buffer;
	end_command_buffer_impl            = mtl_end_command_buffer;
	acquire_next_image_impl            = mtl_acquire_next_image;
	create_shader_impl                 = mtl_create_shader;
	destroy_shader_impl                = mtl_destroy_shader;
	create_descriptor_set_layout_impl  = mtl_create_descriptor_set_layout;
	destroy_descriptor_set_layout_impl = mtl_destroy_descriptor_set_layout;
	create_compute_pipeline_impl       = mtl_create_compute_pipeline;
	create_graphics_pipeline_impl      = mtl_create_graphics_pipeline;
	destroy_pipeline_impl              = mtl_destroy_pipeline;
	create_buffer_impl                 = mtl_create_buffer;
	destroy_buffer_impl                = mtl_destroy_buffer;
	map_memory_impl                    = mtl_map_memory;
	unmap_memory_impl                  = mtl_unmap_memory;
	create_sampler_impl                = mtl_create_sampler;
	destroy_sampler_impl               = mtl_destroy_sampler;
	create_image_impl                  = mtl_create_image;
	destroy_image_impl                 = mtl_destroy_image;
	create_descriptor_set_impl         = mtl_create_descriptor_set;
	destroy_descriptor_set_impl        = mtl_destroy_descriptor_set;
	update_descriptor_set_impl         = mtl_update_descriptor_set;
	cmd_begin_render_pass_impl         = mtl_cmd_begin_render_pass;
	cmd_end_render_pass_impl           = mtl_cmd_end_render_pass;
	cmd_barrier_impl                   = mtl_cmd_barrier;
	cmd_set_scissor_impl               = mtl_cmd_set_scissor;
	cmd_set_viewport_impl              = mtl_cmd_set_viewport;
	cmd_bind_pipeline_impl             = mtl_cmd_bind_pipeline;
	cmd_draw_impl                      = mtl_cmd_draw;
	cmd_draw_indexed_impl              = mtl_cmd_draw_indexed;
	cmd_bind_vertex_buffer_impl        = mtl_cmd_bind_vertex_buffer;
	cmd_bind_index_buffer_u16_impl     = mtl_cmd_bind_index_buffer_u16;
	cmd_bind_index_buffer_u32_impl     = mtl_cmd_bind_index_buffer_u32;
	cmd_copy_buffer_impl               = mtl_cmd_copy_buffer;
	cmd_copy_buffer_to_image_impl      = mtl_cmd_copy_buffer_to_image;
	cmd_bind_descriptor_set_impl       = mtl_cmd_bind_descriptor_set;
	cmd_dispatch_impl                  = mtl_cmd_dispatch;
	cmd_push_constants_impl            = mtl_cmd_push_constants;
	cmd_draw_indexed_indirect_impl     = mtl_cmd_draw_indexed_indirect;

	FT_INIT_INTERNAL( renderer_backend, *p, MetalRendererBackend );
	struct Window* w         = info->wsi_info->window;
	renderer_backend->window = w->handle;
}

#endif
