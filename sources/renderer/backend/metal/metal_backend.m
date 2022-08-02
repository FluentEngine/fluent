#ifdef METAL_BACKEND

#ifndef METAL_BACKEND_INCLUDE_OBJC
#define METAL_BACKEND_INCLUDE_OBJC
#endif
#include <hashmap_c/hashmap_c.h>
#include <SDL.h>
#include <tiny_image_format/tinyimageformat_apis.h>
#include "wsi/wsi.h"
#include "os/window.h"
#include "../renderer_private.h"
#include "metal_backend.h"

FT_INLINE MTLPixelFormat
to_mtl_format( enum ft_format format )
{
	return ( MTLPixelFormat ) TinyImageFormat_ToMTLPixelFormat(
	    ( TinyImageFormat ) format );
}

FT_INLINE MTLLoadAction
to_mtl_load_action( enum ft_attachment_load_op load_op )
{
	switch ( load_op )
	{
	case FT_ATTACHMENT_LOAD_OP_LOAD: return MTLLoadActionLoad;
	case FT_ATTACHMENT_LOAD_OP_CLEAR: return MTLLoadActionClear;
	case FT_ATTACHMENT_LOAD_OP_DONT_CARE: return MTLLoadActionDontCare;
	default: FT_ASSERT( false ); return ( MTLLoadAction ) -1;
	}
}

FT_INLINE MTLStorageMode
to_mtl_storage_mode( enum ft_memory_usage usage )
{
	// TODO:
	switch ( usage )
	{
	case FT_MEMORY_USAGE_CPU_TO_GPU: return MTLStorageModeShared;
	case FT_MEMORY_USAGE_GPU_ONLY: return MTLStorageModeManaged;
	default: return MTLStorageModeManaged;
	}
}

FT_INLINE MTLTextureUsage
to_mtl_texture_usage( enum ft_descriptor_type type )
{
	MTLTextureUsage usage = 0;
	if ( ( bool ) ( type & FT_DESCRIPTOR_TYPE_SAMPLED_IMAGE ) )
		usage |= MTLTextureUsageShaderRead;
	if ( ( bool ) ( type & FT_DESCRIPTOR_TYPE_STORAGE_IMAGE ) )
		usage |= MTLTextureUsageShaderWrite;
	if ( ( bool ) ( type & FT_DESCRIPTOR_TYPE_COLOR_ATTACHMENT ) )
		usage |= MTLTextureUsageRenderTarget;
	if ( ( bool ) ( type & FT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT ) )
		usage |= MTLTextureUsageRenderTarget;
	if ( ( bool ) ( type & FT_DESCRIPTOR_TYPE_DEPTH_STENCIL_ATTACHMENT ) )
		usage |= MTLTextureUsageRenderTarget;
	return usage;
}

FT_INLINE MTLVertexFormat
to_mtl_vertex_format( enum ft_format format )
{
	switch ( format )
	{
	case FT_FORMAT_R8G8_UINT: return MTLVertexFormatUChar2;
	case FT_FORMAT_R8G8B8_UINT: return MTLVertexFormatUChar3;
	case FT_FORMAT_R8G8B8A8_UINT: return MTLVertexFormatUChar4;

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
	default: FT_ASSERT( false ); return MTLVertexFormatInvalid;
	}
}

FT_INLINE MTLVertexStepFunction
to_mtl_vertex_step_function( enum ft_vertex_input_rate input_rate )
{
	switch ( input_rate )
	{
	case FT_VERTEX_INPUT_RATE_VERTEX: return MTLVertexStepFunctionPerVertex;
	case FT_VERTEX_INPUT_RATE_INSTANCE: return MTLVertexStepFunctionPerInstance;
	default: FT_ASSERT( false ); return ( MTLVertexStepFunction ) -1;
	}
}

FT_INLINE MTLSamplerMinMagFilter
to_mtl_sampler_min_mag_filter( enum ft_filter filter )
{
	switch ( filter )
	{
	case FT_FILTER_LINEAR: return MTLSamplerMinMagFilterLinear;
	case FT_FILTER_NEAREST: return MTLSamplerMinMagFilterNearest;
	default: FT_ASSERT( false ); return ( MTLSamplerMinMagFilter ) -1;
	}
}

FT_INLINE MTLSamplerMipFilter
to_mtl_sampler_mip_filter( enum ft_sampler_mipmap_mode mode )
{
	switch ( mode )
	{
	case FT_SAMPLER_MIPMAP_MODE_LINEAR: return MTLSamplerMipFilterLinear;
	case FT_SAMPLER_MIPMAP_MODE_NEAREST: return MTLSamplerMipFilterNearest;
	default: FT_ASSERT( false ); return ( MTLSamplerMipFilter ) -1;
	}
}

FT_INLINE MTLSamplerAddressMode
to_mtl_sampler_address_mode( enum ft_sampler_address_mode mode )
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
	default: FT_ASSERT( false ); return ( MTLSamplerAddressMode ) -1;
	}
}

FT_INLINE MTLCompareFunction
to_mtl_compare_function( enum ft_compare_op op )
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
	default: FT_ASSERT( false ); return ( MTLCompareFunction ) -1;
	}
}

FT_INLINE MTLPrimitiveType
to_mtl_primitive_type( enum ft_primitive_topology topology )
{
	switch ( topology )
	{
	case FT_PRIMITIVE_TOPOLOGY_POINT_LIST: return MTLPrimitiveTypePoint;
	case FT_PRIMITIVE_TOPOLOGY_LINE_LIST: return MTLPrimitiveTypeLine;
	case FT_PRIMITIVE_TOPOLOGY_LINE_STRIP: return MTLPrimitiveTypeLineStrip;
	case FT_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: return MTLPrimitiveTypeTriangle;
	case FT_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
		return MTLPrimitiveTypeTriangleStrip;
	default: FT_ASSERT( false ); return ( MTLPrimitiveType ) -1;
	}
}

static void
mtl_destroy_renderer_backend( struct ft_instance* ibackend )
{
	FT_ASSERT( ibackend );
	FT_FROM_HANDLE( backend, ibackend, MetalRendererBackend );
	free( backend );
}

static void
mtl_create_device( const struct ft_instance* ibackend,
                   const struct ft_device_info*      info,
                   struct ft_device**                p )
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
mtl_destroy_device( struct ft_device* idevice )
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
mtl_create_queue( const struct ft_device*     idevice,
                  const struct ft_queue_info* info,
                  struct ft_queue**           p )
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
mtl_destroy_queue( struct ft_queue* iqueue )
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
mtl_queue_wait_idle( const struct ft_queue* iqueue )
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
mtl_queue_submit( const struct ft_queue*             iqueue,
                  const struct ft_queue_submit_info* info )
{
}

static void
mtl_immediate_submit( const struct ft_queue*    iqueue,
                      struct ft_command_buffer* cmd )
{
	struct ft_queue_submit_info queue_submit_info = { 0 };
	queue_submit_info.command_buffer_count        = 1;
	queue_submit_info.command_buffers             = &cmd;
	ft_queue_submit( iqueue, &queue_submit_info );
	ft_queue_wait_idle( iqueue );
}

static void
mtl_queue_present( const struct ft_queue*              iqueue,
                   const struct ft_queue_present_info* info )
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
mtl_create_semaphore( const struct ft_device* idevice, struct ft_semaphore** p )
{
	FT_ASSERT( p );

	FT_INIT_INTERNAL( semaphore, *p, MetalSemaphore );
}

static void
mtl_destroy_semaphore( const struct ft_device* idevice,
                       struct ft_semaphore*    isemaphore )
{
	FT_ASSERT( isemaphore );
	FT_FROM_HANDLE( semaphore, isemaphore, MetalSemaphore );
	free( semaphore );
}

static void
mtl_create_fence( const struct ft_device* idevice, struct ft_fence** p )
{
	FT_ASSERT( idevice );
	FT_ASSERT( p );

	FT_INIT_INTERNAL( fence, *p, MetalFence );
}

static void
mtl_destroy_fence( const struct ft_device* idevice, struct ft_fence* ifence )
{
	FT_ASSERT( idevice );
	FT_ASSERT( ifence );
	FT_FROM_HANDLE( fence, ifence, MetalFence );
	free( fence );
}

static void
mtl_wait_for_fences( const struct ft_device* idevice,
                     uint32_t                count,
                     struct ft_fence**       ifences )
{
}

static void
mtl_reset_fences( const struct ft_device* idevice,
                  uint32_t                count,
                  struct ft_fence**       ifences )
{
}

static void
mtl_create_swapchain( const struct ft_device*         idevice,
                      const struct ft_swapchain_info* info,
                      struct ft_swapchain**           p )
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

		swapchain->interface.images = calloc( swapchain->interface.image_count,
		                                      sizeof( struct ft_image* ) );

		for ( uint32_t i = 0; i < swapchain->interface.image_count; i++ )
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
mtl_resize_swapchain( const struct ft_device* idevice,
                      struct ft_swapchain*    iswapchain,
                      uint32_t                width,
                      uint32_t                height )
{
	FT_ASSERT( idevice );
	FT_ASSERT( iswapchain );

	FT_FROM_HANDLE( swapchain, iswapchain, MetalSwapchain );

	swapchain->interface.width  = width;
	swapchain->interface.height = height;
}

static void
mtl_destroy_swapchain( const struct ft_device* idevice,
                       struct ft_swapchain*    iswapchain )
{
	FT_ASSERT( iswapchain );

	FT_FROM_HANDLE( swapchain, iswapchain, MetalSwapchain );

	for ( uint32_t i = 0; i < swapchain->interface.image_count; i++ )
	{
		FT_FROM_HANDLE( image, swapchain->interface.images[ i ], MetalImage );
		free( image );
	}

	free( swapchain->interface.images );
	free( swapchain );
}

static void
mtl_create_command_pool( const struct ft_device*            idevice,
                         const struct ft_command_pool_info* info,
                         struct ft_command_pool**           p )
{
	FT_ASSERT( idevice );
	FT_ASSERT( info );
	FT_ASSERT( p );

	FT_INIT_INTERNAL( cmd_pool, *p, MetalCommandPool );

	cmd_pool->interface.queue = info->queue;
}

static void
mtl_destroy_command_pool( const struct ft_device* idevice,
                          struct ft_command_pool* icommand_pool )
{
	FT_ASSERT( idevice );
	FT_ASSERT( icommand_pool );

	FT_FROM_HANDLE( cmd_pool, icommand_pool, MetalCommandPool );

	free( cmd_pool );
}

static void
mtl_create_command_buffers( const struct ft_device*       idevice,
                            const struct ft_command_pool* icommand_pool,
                            uint32_t                      count,
                            struct ft_command_buffer**    p )
{
	FT_ASSERT( idevice );
	FT_ASSERT( icommand_pool );
	FT_ASSERT( p );

	FT_FROM_HANDLE( cmd_pool, icommand_pool, MetalCommandPool );

	for ( uint32_t i = 0; i < count; i++ )
	{
		FT_INIT_INTERNAL( cmd, p[ i ], MetalCommandBuffer );
		cmd->interface.queue = cmd_pool->interface.queue;
	}
}

static void
mtl_free_command_buffers( const struct ft_device*       idevice,
                          const struct ft_command_pool* icommand_pool,
                          uint32_t                      count,
                          struct ft_command_buffer**    icommand_buffers )
{
}

static void
mtl_destroy_command_buffers( const struct ft_device*       idevice,
                             const struct ft_command_pool* icmd_pool,
                             uint32_t                      count,
                             struct ft_command_buffer**    icommand_buffers )
{
	FT_ASSERT( idevice );
	FT_ASSERT( icmd_pool );

	for ( uint32_t i = 0; i < count; i++ )
	{
		FT_FROM_HANDLE( cmd, icommand_buffers[ i ], MetalCommandBuffer );
		free( cmd );
	}
}

static void
mtl_begin_command_buffer( const struct ft_command_buffer* icmd )
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
mtl_end_command_buffer( const struct ft_command_buffer* icmd )
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
mtl_acquire_next_image( const struct ft_device*    idevice,
                        const struct ft_swapchain* iswapchain,
                        const struct ft_semaphore* isemaphore,
                        const struct ft_fence*     ifence,
                        uint32_t*                  image_index )
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
mtl_create_function( const struct MetalDevice*           device,
                     struct MetalShader*                 shader,
                     enum ft_shader_stage                stage,
                     const struct ft_shader_module_info* info )
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
mtl_create_shader( const struct ft_device* idevice,
                   struct ft_shader_info*  info,
                   struct ft_shader**      p )
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

		ft_mtl_reflect( idevice, info, &shader->interface );
	}
}

static void
mtl_destroy_shader( const struct ft_device* idevice, struct ft_shader* ishader )
{
	@autoreleasepool
	{
		FT_ASSERT( idevice );
		FT_ASSERT( ishader );

		FT_FROM_HANDLE( shader, ishader, MetalShader );

		for ( uint32_t i = 0; i < FT_SHADER_STAGE_COUNT; ++i )
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
mtl_create_descriptor_set_layout( const struct ft_device*           idevice,
                                  struct ft_shader*                 ishader,
                                  struct ft_descriptor_set_layout** p )
{
	FT_INIT_INTERNAL( layout, *p, MetalDescriptorSetLayout );

	struct ft_reflection_data reflect_data = {
	    .binding_count = ishader->reflect_data.binding_count,
	    .binding_map   = hashmap_new( sizeof( struct ft_binding_map_item ),
                                    0,
                                    0,
                                    0,
                                    binding_map_hash,
                                    binding_map_compare,
                                    NULL,
                                    NULL ),
	};

	if ( reflect_data.binding_count != 0 )
	{
		size_t iter = 0;
		void*  item;
		while (
		    hashmap_iter( ishader->reflect_data.binding_map, &iter, &item ) )
		{
			hashmap_set( reflect_data.binding_map, item );
		}

		FT_ALLOC_HEAP_ARRAY( struct ft_binding,
		                     bindings,
		                     ishader->reflect_data.binding_count );
		reflect_data.bindings = bindings;
		for ( uint32_t i = 0; i < ishader->reflect_data.binding_count; ++i )
		{
			reflect_data.bindings[ i ] = ishader->reflect_data.bindings[ i ];
		}
	}
	layout->interface.reflection_data = reflect_data;
}

static void
mtl_destroy_descriptor_set_layout( const struct ft_device*          idevice,
                                   struct ft_descriptor_set_layout* ilayout )
{
	FT_FROM_HANDLE( layout, ilayout, MetalDescriptorSetLayout );

	free( layout );
}

static void
mtl_create_compute_pipeline( const struct ft_device*        idevice,
                             const struct ft_pipeline_info* info,
                             struct ft_pipeline**           p )
{
}

static void
mtl_create_graphics_pipeline( const struct ft_device*        idevice,
                              const struct ft_pipeline_info* info,
                              struct ft_pipeline**           p )
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

		for ( uint32_t i = 0; i < FT_SHADER_STAGE_COUNT; ++i )
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

		for ( uint32_t i = 0; i < info->vertex_layout.binding_info_count; ++i )
		{
			// TODO: rewrite more elegant? binding should not conflict with
			// uniform bindings
			uint32_t binding = info->vertex_layout.binding_infos[ i ].binding +
			                   FT_MAX_VERTEX_BINDING_COUNT;
			vertex_layout.layouts[ binding ].stepFunction =
			    to_mtl_vertex_step_function(
			        info->vertex_layout.binding_infos[ i ].input_rate );
			vertex_layout.layouts[ binding ].stride =
			    info->vertex_layout.binding_infos[ i ].stride;
		}

		for ( uint32_t i = 0; i < info->vertex_layout.attribute_info_count;
		      ++i )
		{
			uint32_t location =
			    info->vertex_layout.attribute_infos[ i ].location;
			vertex_layout.attributes[ location ].format = to_mtl_vertex_format(
			    info->vertex_layout.attribute_infos[ i ].format );
			vertex_layout.attributes[ location ].offset =
			    info->vertex_layout.attribute_infos[ i ].offset;
			vertex_layout.attributes[ location ].bufferIndex =
			    info->vertex_layout.attribute_infos[ i ].binding +
			    FT_MAX_VERTEX_BINDING_COUNT;
		}

		pipeline->pipeline_descriptor.vertexDescriptor = vertex_layout;

		for ( uint32_t i = 0; i < info->color_attachment_count; ++i )
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
mtl_destroy_pipeline( const struct ft_device* idevice,
                      struct ft_pipeline*     ipipeline )
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
mtl_create_buffer( const struct ft_device*      idevice,
                   const struct ft_buffer_info* info,
                   struct ft_buffer**           p )
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
mtl_destroy_buffer( const struct ft_device* idevice, struct ft_buffer* ibuffer )
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
mtl_map_memory( const struct ft_device* idevice, struct ft_buffer* ibuffer )
{
	@autoreleasepool
	{
		FT_FROM_HANDLE( buffer, ibuffer, MetalBuffer );
		buffer->interface.mapped_memory = buffer->buffer.contents;
		return buffer->interface.mapped_memory;
	}
}

static void
mtl_unmap_memory( const struct ft_device* idevice, struct ft_buffer* ibuffer )
{
	ibuffer->mapped_memory = NULL;
}

static void
mtl_create_sampler( const struct ft_device*       idevice,
                    const struct ft_sampler_info* info,
                    struct ft_sampler**           p )
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
mtl_destroy_sampler( const struct ft_device* idevice,
                     struct ft_sampler*      isampler )
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
mtl_create_image( const struct ft_device*     idevice,
                  const struct ft_image_info* info,
                  struct ft_image**           p )
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
mtl_destroy_image( const struct ft_device* idevice, struct ft_image* iimage )
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

FT_INLINE void
count_binding_types( const struct ft_reflection_data* reflection,
                     uint32_t*                        sampler_binding_count,
                     struct MetalSamplerBinding**     sampler_bindings,
                     uint32_t*                        image_binding_count,
                     struct MetalImageBinding**       image_bindings,
                     uint32_t*                        buffer_binding_count,
                     struct MetalBufferBinding**      buffer_bindings )
{
	uint32_t           binding_count = reflection->binding_count;
	struct ft_binding* bindings      = reflection->bindings;

	uint32_t s = 0;
	uint32_t i = 0;
	uint32_t b = 0;

	sampler_bindings =
	    calloc( binding_count, sizeof( struct MetalSamplerBinding* ) );
	image_bindings =
	    calloc( binding_count, sizeof( struct MetalImageBinding* ) );
	buffer_bindings =
	    calloc( binding_count, sizeof( struct ft_bufferBinding* ) );

	for ( uint32_t b = 0; b < binding_count; ++b )
	{
		struct ft_binding* binding = &bindings[ b ];
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
mtl_create_descriptor_set( const struct ft_device*              idevice,
                           const struct ft_descriptor_set_info* info,
                           struct ft_descriptor_set**           p )
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
mtl_destroy_descriptor_set( const struct ft_device*   idevice,
                            struct ft_descriptor_set* iset )
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
mtl_update_descriptor_set( const struct ft_device*           idevice,
                           struct ft_descriptor_set*         iset,
                           uint32_t                          count,
                           const struct ft_descriptor_write* writes )
{
#if 0
	@autoreleasepool
	{
		FT_FROM_HANDLE( set, iset, MetalDescriptorSet );

		const auto& binding_map =
		    set->interface.layout->reflection_data.binding_map;
		const auto& bindings = set->interface.layout->reflection_data.bindings;

		for ( uint32_t i = 0; i < count; ++i )
		{
			FT_ASSERT( binding_map.find( writes[ i ].descriptor_name ) !=
			           binding_map.cend() );

			const Binding* binding =
			    &bindings[ binding_map.at( writes[ i ].descriptor_name ) ];

			switch ( binding->descriptor_type )
			{
			case FT_DESCRIPTOR_TYPE_SAMPLER:
			{
				for ( uint32_t j = 0; j < set->sampler_binding_count; ++j )
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
				for ( uint32_t j = 0; j < set->image_binding_count; ++j )
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
				for ( uint32_t j = 0; j < set->buffer_binding_count; ++j )
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
mtl_cmd_begin_render_pass( const struct ft_command_buffer*         icmd,
                           const struct ft_render_pass_begin_info* info )
{
	@autoreleasepool
	{
		FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );

		// TODO: fix
		MTLRenderPassDescriptor* pass_descriptor =
		    [MTLRenderPassDescriptor renderPassDescriptor];

		for ( uint32_t i = 0; i < info->color_attachment_count; ++i )
		{
			FT_FROM_HANDLE( image,
			                info->color_attachments[ i ].image,
			                MetalImage );
			pass_descriptor.colorAttachments[ i ].loadAction =
			    to_mtl_load_action( info->color_attachments[ i ].load_op );
			pass_descriptor.colorAttachments[ i ].storeAction =
			    MTLStoreActionStore;

			pass_descriptor.colorAttachments[ i ].texture = image->texture;

			pass_descriptor.colorAttachments[ i ].clearColor =
			    MTLClearColorMake(
			        info->color_attachments[ i ].clear_value.color[ 0 ],
			        info->color_attachments[ i ].clear_value.color[ 1 ],
			        info->color_attachments[ i ].clear_value.color[ 2 ],
			        info->color_attachments[ i ].clear_value.color[ 3 ] );
			// TODO:
			pass_descriptor.renderTargetWidth  = image->interface.width;
			pass_descriptor.renderTargetHeight = image->interface.height;
			pass_descriptor.defaultRasterSampleCount =
			    image->interface.sample_count;
		}

		if ( info->depth_attachment.image != NULL )
		{
			FT_FROM_HANDLE( image, info->depth_attachment.image, MetalImage );
			pass_descriptor.depthAttachment.texture = image->texture;
			pass_descriptor.depthAttachment.loadAction =
			    to_mtl_load_action( info->depth_attachment.load_op );
			pass_descriptor.depthAttachment.storeAction = MTLStoreActionStore;
			pass_descriptor.depthAttachment.texture     = image->texture;

			pass_descriptor.depthAttachment.clearDepth =
			    info->depth_attachment.clear_value.depth_stencil.depth;

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
mtl_cmd_end_render_pass( const struct ft_command_buffer* icmd )
{
	@autoreleasepool
	{
		FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );

		[cmd->encoder endEncoding];
		cmd->encoder = nil;
	}
}

static void
mtl_cmd_barrier( const struct ft_command_buffer* icmd,
                 uint32_t                        memory_barrier_count,
                 const struct ft_memory_barrier* memory_barriers,
                 uint32_t                        buffer_barriers_count,
                 const struct ft_buffer_barrier* buffer_barriers,
                 uint32_t                        image_barriers_count,
                 const struct ft_image_barrier*  image_barriers ) {};

static void
mtl_cmd_set_scissor( const struct ft_command_buffer* icmd,
                     int32_t                         x,
                     int32_t                         y,
                     uint32_t                        width,
                     uint32_t                        height )
{
	FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );

	[cmd->encoder setScissorRect:( MTLScissorRect ) { ( uint32_t ) x,
	                                                  ( uint32_t ) y,
	                                                  width,
	                                                  height }];
}

static void
mtl_cmd_set_viewport( const struct ft_command_buffer* icmd,
                      float                           x,
                      float                           y,
                      float                           width,
                      float                           height,
                      float                           min_depth,
                      float                           max_depth )
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
mtl_cmd_bind_pipeline( const struct ft_command_buffer* icmd,
                       const struct ft_pipeline*       ipipeline )
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
mtl_cmd_draw( const struct ft_command_buffer* icmd,
              uint32_t                        vertex_count,
              uint32_t                        instance_count,
              uint32_t                        first_vertex,
              uint32_t                        first_instance )
{
	FT_ASSERT( icmd );

	FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );

	[cmd->encoder drawPrimitives:cmd->primitive_type
	                 vertexStart:first_vertex
	                 vertexCount:vertex_count];
}

static void
mtl_cmd_draw_indexed( const struct ft_command_buffer* icmd,
                      uint32_t                        index_count,
                      uint32_t                        instance_count,
                      uint32_t                        first_index,
                      int32_t                         vertex_offset,
                      uint32_t                        first_instance )
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
mtl_cmd_bind_vertex_buffer( const struct ft_command_buffer* icmd,
                            const struct ft_buffer*         ibuffer,
                            const uint64_t                  offset )
{
	FT_ASSERT( icmd );
	FT_ASSERT( ibuffer );

	FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
	FT_FROM_HANDLE( buffer, ibuffer, MetalBuffer );

	[cmd->encoder setVertexBuffer:buffer->buffer
	                       offset:offset
	                      atIndex:FT_MAX_VERTEX_BINDING_COUNT];
}

static void
mtl_cmd_bind_index_buffer_uint16_t( const struct ft_command_buffer* icmd,
                                    const struct ft_buffer*         ibuffer,
                                    const uint64_t                  offset )
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
mtl_cmd_bind_index_buffer_uint32_t( const struct ft_command_buffer* icmd,
                                    const struct ft_buffer*         ibuffer,
                                    uint64_t                        offset )
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
mtl_cmd_copy_buffer( const struct ft_command_buffer* icmd,
                     const struct ft_buffer*         isrc,
                     uint64_t                        src_offset,
                     struct ft_buffer*               idst,
                     uint64_t                        dst_offset,
                     uint64_t                        size )
{
	FT_FROM_HANDLE( src, isrc, MetalBuffer );
	FT_FROM_HANDLE( dst, idst, MetalBuffer );

	uint8_t* src_ptr = src->buffer.contents;
	uint8_t* dst_ptr = dst->buffer.contents;

	memcpy( dst_ptr + dst_offset, src_ptr + src_offset, size );
}

static void
mtl_cmd_copy_buffer_to_image( const struct ft_command_buffer* icmd,
                              const struct ft_buffer*         isrc,
                              uint64_t                        src_offset,
                              struct ft_image*                idst )
{
	FT_FROM_HANDLE( src, isrc, MetalBuffer );
	FT_FROM_HANDLE( dst, idst, MetalImage );

	uint8_t* src_ptr = src->buffer.contents;

	MTLRegion region = {
	    { 0, 0, 0 },
	    { dst->interface.width, dst->interface.height, dst->interface.depth } };

	NSUInteger bytesPerRow = 4 * dst->interface.width;
	[dst->texture replaceRegion:region
	                mipmapLevel:0
	                  withBytes:src_ptr + src_offset
	                bytesPerRow:bytesPerRow];
}

static void
mtl_cmd_dispatch( const struct ft_command_buffer* icmd,
                  uint32_t                        group_count_x,
                  uint32_t                        group_count_y,
                  uint32_t                        group_count_z )
{
}

static void
mtl_cmd_push_constants( const struct ft_command_buffer* icmd,
                        const struct ft_pipeline*       ipipeline,
                        uint32_t                        offset,
                        uint32_t                        size,
                        const void*                     data )
{
}

static void
mtl_cmd_blit_image( const struct ft_command_buffer* icmd,
                    const struct ft_image*          isrc,
                    enum ft_resource_state          src_state,
                    struct ft_image*                idst,
                    enum ft_resource_state          dst_state,
                    enum ft_filter                  filter )
{
}

static void
mtl_cmd_draw_indexed_indirect( const struct ft_command_buffer* icmd,
                               const struct ft_buffer*         ibuffer,
                               uint64_t                        offset,
                               uint32_t                        draw_count,
                               uint32_t                        stride )
{
}

static void
mtl_cmd_bind_descriptor_set( const struct ft_command_buffer* icmd,
                             uint32_t                        first_set,
                             const struct ft_descriptor_set* iset,
                             const struct ft_pipeline*       ipipeline )
{
	FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
	FT_FROM_HANDLE( set, iset, MetalDescriptorSet );

	for ( uint32_t i = 0; i < set->sampler_binding_count; ++i )
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

	for ( uint32_t i = 0; i < set->image_binding_count; ++i )
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

	for ( uint32_t i = 0; i < set->buffer_binding_count; ++i )
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
mtl_create_renderer_backend( const struct ft_instance_info* info,
                             struct ft_instance**           p )
{
	FT_UNUSED( info );
	FT_ASSERT( p );

	destroy_renderer_backend_impl       = mtl_destroy_renderer_backend;
	create_device_impl                  = mtl_create_device;
	destroy_device_impl                 = mtl_destroy_device;
	create_queue_impl                   = mtl_create_queue;
	destroy_queue_impl                  = mtl_destroy_queue;
	queue_wait_idle_impl                = mtl_queue_wait_idle;
	queue_submit_impl                   = mtl_queue_submit;
	immediate_submit_impl               = mtl_immediate_submit;
	queue_present_impl                  = mtl_queue_present;
	create_semaphore_impl               = mtl_create_semaphore;
	destroy_semaphore_impl              = mtl_destroy_semaphore;
	create_fence_impl                   = mtl_create_fence;
	destroy_fence_impl                  = mtl_destroy_fence;
	wait_for_fences_impl                = mtl_wait_for_fences;
	reset_fences_impl                   = mtl_reset_fences;
	create_swapchain_impl               = mtl_create_swapchain;
	resize_swapchain_impl               = mtl_resize_swapchain;
	destroy_swapchain_impl              = mtl_destroy_swapchain;
	create_command_pool_impl            = mtl_create_command_pool;
	destroy_command_pool_impl           = mtl_destroy_command_pool;
	create_command_buffers_impl         = mtl_create_command_buffers;
	free_command_buffers_impl           = mtl_free_command_buffers;
	destroy_command_buffers_impl        = mtl_destroy_command_buffers;
	begin_command_buffer_impl           = mtl_begin_command_buffer;
	end_command_buffer_impl             = mtl_end_command_buffer;
	acquire_next_image_impl             = mtl_acquire_next_image;
	create_shader_impl                  = mtl_create_shader;
	destroy_shader_impl                 = mtl_destroy_shader;
	create_descriptor_set_layout_impl   = mtl_create_descriptor_set_layout;
	destroy_descriptor_set_layout_impl  = mtl_destroy_descriptor_set_layout;
	create_compute_pipeline_impl        = mtl_create_compute_pipeline;
	create_graphics_pipeline_impl       = mtl_create_graphics_pipeline;
	destroy_pipeline_impl               = mtl_destroy_pipeline;
	create_buffer_impl                  = mtl_create_buffer;
	destroy_buffer_impl                 = mtl_destroy_buffer;
	map_memory_impl                     = mtl_map_memory;
	unmap_memory_impl                   = mtl_unmap_memory;
	create_sampler_impl                 = mtl_create_sampler;
	destroy_sampler_impl                = mtl_destroy_sampler;
	create_image_impl                   = mtl_create_image;
	destroy_image_impl                  = mtl_destroy_image;
	create_descriptor_set_impl          = mtl_create_descriptor_set;
	destroy_descriptor_set_impl         = mtl_destroy_descriptor_set;
	update_descriptor_set_impl          = mtl_update_descriptor_set;
	cmd_begin_render_pass_impl          = mtl_cmd_begin_render_pass;
	cmd_end_render_pass_impl            = mtl_cmd_end_render_pass;
	cmd_barrier_impl                    = mtl_cmd_barrier;
	cmd_set_scissor_impl                = mtl_cmd_set_scissor;
	cmd_set_viewport_impl               = mtl_cmd_set_viewport;
	cmd_bind_pipeline_impl              = mtl_cmd_bind_pipeline;
	cmd_draw_impl                       = mtl_cmd_draw;
	cmd_draw_indexed_impl               = mtl_cmd_draw_indexed;
	cmd_bind_vertex_buffer_impl         = mtl_cmd_bind_vertex_buffer;
	cmd_bind_index_buffer_uint16_t_impl = mtl_cmd_bind_index_buffer_uint16_t;
	cmd_bind_index_buffer_uint32_t_impl = mtl_cmd_bind_index_buffer_uint32_t;
	cmd_copy_buffer_impl                = mtl_cmd_copy_buffer;
	cmd_copy_buffer_to_image_impl       = mtl_cmd_copy_buffer_to_image;
	cmd_bind_descriptor_set_impl        = mtl_cmd_bind_descriptor_set;
	cmd_dispatch_impl                   = mtl_cmd_dispatch;
	cmd_push_constants_impl             = mtl_cmd_push_constants;
	cmd_draw_indexed_indirect_impl      = mtl_cmd_draw_indexed_indirect;

	FT_INIT_INTERNAL( renderer_backend, *p, MetalRendererBackend );
	struct ft_window* w      = info->wsi_info->window;
	renderer_backend->window = w->handle;
}

#endif
