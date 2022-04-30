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
    case AttachmentLoadOp::eLoad: return MTLLoadActionLoad;
    case AttachmentLoadOp::eClear: return MTLLoadActionClear;
    case AttachmentLoadOp::eDontCare: return MTLLoadActionDontCare;
    default: FT_ASSERT( false ); return MTLLoadAction( -1 );
    }
}

static inline u32
to_mtl_sample_count( SampleCount sample_count )
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
    default: FT_ASSERT( false ); return static_cast<u32>( -1 );
    }
}

static inline MTLStorageMode
to_mtl_storage_mode( MemoryUsage usage )
{
    // TODO:
    switch ( usage )
    {
    case MemoryUsage::eCpuToGpu: return MTLStorageModeShared;
    case MemoryUsage::eGpuOnly: return MTLStorageModeManaged;
    default: return MTLStorageModeManaged;
    }
}

static inline MTLTextureUsage
to_mtl_texture_usage( DescriptorType type )
{
    switch ( type )
    {
    case DescriptorType::eSampledImage: return MTLTextureUsageShaderRead;
    case DescriptorType::eStorageImage: return MTLTextureUsageShaderWrite;
    case DescriptorType::eColorAttachment:
    case DescriptorType::eInputAttachment:
    case DescriptorType::eDepthStencilAttachment:
        return MTLTextureUsageRenderTarget;
    default: FT_ASSERT( false ); return MTLTextureUsage( -1 );
    }
}

static inline MTLVertexFormat
to_mtl_vertex_format( Format format )
{
    switch ( format )
    {
    case Format::eR8G8Unorm: return MTLVertexFormatUChar2Normalized;
    case Format::eR8G8B8Unorm: return MTLVertexFormatUChar3Normalized;
    case Format::eR8G8B8A8Unorm: return MTLVertexFormatUChar4Normalized;

    case Format::eR8G8Snorm: return MTLVertexFormatChar2Normalized;
    case Format::eR8G8B8Snorm: return MTLVertexFormatChar3Normalized;
    case Format::eR8G8B8A8Snorm: return MTLVertexFormatChar4Normalized;

    case Format::eR16G16Unorm: return MTLVertexFormatUShort2Normalized;
    case Format::eR16G16B16Unorm: return MTLVertexFormatUShort3Normalized;
    case Format::eR16G16B16A16Unorm: return MTLVertexFormatUShort4Normalized;

    case Format::eR16G16Snorm: return MTLVertexFormatShort2Normalized;
    case Format::eR16G16B16Snorm: return MTLVertexFormatShort3Normalized;
    case Format::eR16G16B16A16Snorm: return MTLVertexFormatShort4Normalized;

    case Format::eR16G16Sint: return MTLVertexFormatShort2;
    case Format::eR16G16B16Sint: return MTLVertexFormatShort3;
    case Format::eR16G16B16A16Sint: return MTLVertexFormatShort4;

    case Format::eR16G16Uint: return MTLVertexFormatUShort2;
    case Format::eR16G16B16Uint: return MTLVertexFormatUShort3;
    case Format::eR16G16B16A16Uint: return MTLVertexFormatUShort4;

    case Format::eR16G16Sfloat: return MTLVertexFormatHalf2;
    case Format::eR16G16B16Sfloat: return MTLVertexFormatHalf3;
    case Format::eR16G16B16A16Sfloat: return MTLVertexFormatHalf4;

    case Format::eR32Sfloat: return MTLVertexFormatFloat;
    case Format::eR32G32Sfloat: return MTLVertexFormatFloat2;
    case Format::eR32G32B32Sfloat: return MTLVertexFormatFloat3;
    case Format::eR32G32B32A32Sfloat: return MTLVertexFormatFloat4;

    case Format::eR32Sint: return MTLVertexFormatInt;
    case Format::eR32G32Sint: return MTLVertexFormatInt2;
    case Format::eR32G32B32Sint: return MTLVertexFormatInt3;
    case Format::eR32G32B32A32Sint: return MTLVertexFormatInt4;

    case Format::eR32Uint: return MTLVertexFormatUInt;
    case Format::eR32G32Uint: return MTLVertexFormatUInt2;
    case Format::eR32G32B32Uint: return MTLVertexFormatUInt3;
    case Format::eR32G32B32A32Uint: return MTLVertexFormatUInt4;
    default: FT_ASSERT( false ); return MTLVertexFormatInvalid;
    }
}

static inline MTLVertexStepFunction
to_mtl_vertex_step_function( VertexInputRate input_rate )
{
    switch ( input_rate )
    {
    case VertexInputRate::eVertex: return MTLVertexStepFunctionPerVertex;
    case VertexInputRate::eInstance: return MTLVertexStepFunctionPerInstance;
    default: FT_ASSERT( false ); return MTLVertexStepFunction( -1 );
    }
}

static inline MTLSamplerMinMagFilter
to_mtl_sampler_min_mag_filter( Filter filter )
{
    switch ( filter )
    {
    case Filter::eLinear: return MTLSamplerMinMagFilterLinear;
    case Filter::eNearest: return MTLSamplerMinMagFilterNearest;
    default: FT_ASSERT( false ); return MTLSamplerMinMagFilter( -1 );
    }
}

static inline MTLSamplerMipFilter
to_mtl_sampler_mip_filter( SamplerMipmapMode mode )
{
    switch ( mode )
    {
    case SamplerMipmapMode::eLinear: return MTLSamplerMipFilterLinear;
    case SamplerMipmapMode::eNearest: return MTLSamplerMipFilterNearest;
    default: FT_ASSERT( false ); return MTLSamplerMipFilter( -1 );
    }
}

static inline MTLSamplerAddressMode
to_mtl_sampler_address_mode( SamplerAddressMode mode )
{
    switch ( mode )
    {
    case SamplerAddressMode::eRepeat: return MTLSamplerAddressModeRepeat;
    case SamplerAddressMode::eMirroredRepeat:
        return MTLSamplerAddressModeMirrorRepeat;
    case SamplerAddressMode::eClampToEdge:
        return MTLSamplerAddressModeClampToEdge;
    case SamplerAddressMode::eClampToBorder:
        return MTLSamplerAddressModeClampToEdge;
    default: FT_ASSERT( false ); return MTLSamplerAddressMode( -1 );
    }
}

static inline MTLCompareFunction
to_mtl_compare_function( CompareOp op )
{
    switch ( op )
    {
    case CompareOp::eNever: return MTLCompareFunctionNever;
    case CompareOp::eLess: return MTLCompareFunctionLess;
    case CompareOp::eEqual: return MTLCompareFunctionEqual;
    case CompareOp::eLessOrEqual: return MTLCompareFunctionLessEqual;
    case CompareOp::eGreater: return MTLCompareFunctionGreater;
    case CompareOp::eNotEqual: return MTLCompareFunctionNotEqual;
    case CompareOp::eGreaterOrEqual: return MTLCompareFunctionGreaterEqual;
    case CompareOp::eAlways: return MTLCompareFunctionAlways;
    default: FT_ASSERT( false ); return MTLCompareFunction( -1 );
    }
}

void
mtl_destroy_renderer_backend( RendererBackend* ibackend )
{
    FT_ASSERT( ibackend );
    FT_FROM_HANDLE( backend, ibackend, MetalRendererBackend );
    operator delete( backend, std::nothrow );
}

void
mtl_create_device( const RendererBackend* ibackend,
                   const DeviceDesc*      desc,
                   Device**               p )
{
    FT_ASSERT( p );

    auto device              = new ( std::nothrow ) MetalDevice {};
    device->interface.handle = device;
    *p                       = &device->interface;

    auto* window   = static_cast<SDL_Window*>( get_app_window()->handle );
    device->device = MTLCreateSystemDefaultDevice();
    device->view   = SDL_Metal_CreateView( window );
}

void
mtl_destroy_device( Device* idevice )
{
    FT_ASSERT( idevice );

    FT_FROM_HANDLE( device, idevice, MetalDevice );

    SDL_Metal_DestroyView( device->view );
    [device->device release];
    operator delete( device, std::nothrow );
}

void
mtl_create_queue( const Device* idevice, const QueueDesc* desc, Queue** p )
{
    FT_ASSERT( p );

    FT_FROM_HANDLE( device, idevice, MetalDevice );

    auto queue              = new ( std::nothrow ) MetalQueue {};
    queue->interface.handle = queue;
    *p                      = &queue->interface;

    queue->queue = [device->device newCommandQueue];
}

void
mtl_destroy_queue( Queue* iqueue )
{
    FT_ASSERT( iqueue );

    FT_FROM_HANDLE( queue, iqueue, MetalQueue );

    [queue->queue release];
    operator delete( queue, std::nothrow );
}

void
mtl_queue_wait_idle( const Queue* iqueue )
{
    FT_ASSERT( iqueue );

    FT_FROM_HANDLE( queue, iqueue, MetalQueue );

    id<MTLCommandBuffer> wait_cmd =
        [queue->queue commandBufferWithUnretainedReferences];

    [wait_cmd commit];
    [wait_cmd waitUntilCompleted];
    [wait_cmd release];
}

void
mtl_queue_submit( const Queue* iqueue, const QueueSubmitDesc* desc )
{
}

void
mtl_immediate_submit( const Queue* iqueue, CommandBuffer* cmd )
{
    QueueSubmitDesc queue_submit_desc {};
    queue_submit_desc.command_buffer_count = 1;
    queue_submit_desc.command_buffers      = &cmd;
    queue_submit( iqueue, &queue_submit_desc );
    queue_wait_idle( iqueue );
}

void
mtl_queue_present( const Queue* iqueue, const QueuePresentDesc* desc )
{
    FT_FROM_HANDLE( queue, iqueue, MetalQueue );
    FT_FROM_HANDLE( swapchain, desc->swapchain, MetalSwapchain );

    id<MTLCommandBuffer> present_cmd = [queue->queue commandBuffer];
    [present_cmd presentDrawable:swapchain->drawable];
    [present_cmd commit];
}

void
mtl_create_semaphore( const Device* idevice, Semaphore** p )
{
    FT_ASSERT( p );

    auto semaphore              = new ( std::nothrow ) MetalSemaphore {};
    semaphore->interface.handle = semaphore;
    *p                          = &semaphore->interface;
}

void
mtl_destroy_semaphore( const Device* idevice, Semaphore* isemaphore )
{
    FT_ASSERT( isemaphore );
    FT_FROM_HANDLE( semaphore, isemaphore, MetalSemaphore );
    operator delete( semaphore, std::nothrow );
}

void
mtl_create_fence( const Device* idevice, Fence** p )
{
    FT_ASSERT( idevice );
    FT_ASSERT( p );

    auto fence              = new ( std::nothrow ) MetalFence {};
    fence->interface.handle = fence;
    *p                      = &fence->interface;
}

void
mtl_destroy_fence( const Device* idevice, Fence* ifence )
{
    FT_ASSERT( idevice );
    FT_ASSERT( ifence );
    FT_FROM_HANDLE( fence, ifence, MetalFence );
    operator delete( fence, std::nothrow );
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
                      const SwapchainDesc* desc,
                      Swapchain**          p )
{
    FT_ASSERT( idevice );
    FT_ASSERT( desc );
    FT_ASSERT( p );

    FT_FROM_HANDLE( device, idevice, MetalDevice );

    FT_INIT_INTERNAL( swapchain, *p, MetalSwapchain );

    swapchain->interface.min_image_count = desc->min_image_count;
    swapchain->interface.image_count     = desc->min_image_count;
    swapchain->interface.width           = desc->width;
    swapchain->interface.height          = desc->height;
    swapchain->interface.format          = desc->format;
    swapchain->interface.queue           = desc->queue;
    swapchain->interface.vsync           = desc->vsync;

    swapchain->swapchain = ( CAMetalLayer* ) SDL_Metal_GetLayer( device->view );
    swapchain->swapchain.device             = device->device;
    swapchain->swapchain.pixelFormat        = to_mtl_format( desc->format );
    swapchain->swapchain.displaySyncEnabled = desc->vsync;

    swapchain->interface.image_count =
        swapchain->swapchain.maximumDrawableCount;

    swapchain->interface.images =
        new ( std::nothrow ) Image*[ swapchain->interface.image_count ];

    for ( u32 i = 0; i < swapchain->interface.image_count; i++ )
    {
        FT_INIT_INTERNAL( image, swapchain->interface.images[ i ], MetalImage );

        image->texture                   = nil;
        image->interface.width           = desc->width;
        image->interface.height          = desc->height;
        image->interface.depth           = 1;
        image->interface.format          = desc->format;
        image->interface.layer_count     = 1;
        image->interface.descriptor_type = DescriptorType::eColorAttachment;
        image->interface.mip_level_count = 1;
        image->interface.sample_count    = SampleCount::e1;
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
        operator delete( image, std::nothrow );
    }
    operator delete[]( swapchain->interface.images, std::nothrow );
    operator delete( swapchain, std::nothrow );
}

void
mtl_create_command_pool( const Device*          idevice,
                         const CommandPoolDesc* desc,
                         CommandPool**          p )
{
    FT_ASSERT( idevice );
    FT_ASSERT( desc );
    FT_ASSERT( p );

    FT_INIT_INTERNAL( cmd_pool, *p, MetalCommandPool );

    cmd_pool->interface.queue = desc->queue;
}

void
mtl_destroy_command_pool( const Device* idevice, CommandPool* icommand_pool )
{
    FT_ASSERT( idevice );
    FT_ASSERT( icommand_pool );

    FT_FROM_HANDLE( cmd_pool, icommand_pool, MetalCommandPool );

    operator delete( cmd_pool, std::nothrow );
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
        auto cmd              = new ( std::nothrow ) MetalCommandBuffer {};
        cmd->interface.handle = cmd;
        cmd->interface.queue  = cmd_pool->interface.queue;
        p[ i ]                = &cmd->interface;
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
        operator delete( cmd, std::nothrow );
    }
}

void
mtl_begin_command_buffer( const CommandBuffer* icmd )
{
    FT_ASSERT( icmd );
    FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
    FT_FROM_HANDLE( queue, cmd->interface.queue, MetalQueue );

    cmd->cmd = [queue->queue commandBuffer];
}

void
mtl_end_command_buffer( const CommandBuffer* icmd )
{
    FT_ASSERT( icmd );
    FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
    [cmd->cmd commit];
    [cmd->cmd release];
}

void
mtl_acquire_next_image( const Device*    idevice,
                        const Swapchain* iswapchain,
                        const Semaphore* isemaphore,
                        const Fence*     ifence,
                        u32*             image_index )
{
    FT_ASSERT( idevice );
    FT_ASSERT( iswapchain );

    FT_FROM_HANDLE( swapchain, iswapchain, MetalSwapchain );
    FT_FROM_HANDLE(
        image,
        swapchain->interface.images[ swapchain->current_image_index ],
        MetalImage );

    swapchain->drawable = swapchain->swapchain.nextDrawable;
    image->texture      = swapchain->drawable.texture;

    *image_index                   = swapchain->current_image_index;
    swapchain->current_image_index = ( swapchain->current_image_index + 1 ) %
                                     swapchain->interface.image_count;
}

void
mtl_create_render_pass( const Device*         idevice,
                        const RenderPassDesc* desc,
                        RenderPass**          p )
{
    FT_ASSERT( p );

    FT_INIT_INTERNAL( render_pass, *p, MetalRenderPass );

    MTLRenderPassDescriptor* pass =
        [MTLRenderPassDescriptor renderPassDescriptor];

    render_pass->render_pass = pass;

    render_pass->interface.color_attachment_count =
        desc->color_attachment_count;
    render_pass->interface.width  = desc->width;
    render_pass->interface.height = desc->height;
    render_pass->interface.sample_count =
        desc->color_attachments[ 0 ]->sample_count;

    pass.renderTargetWidth  = desc->width;
    pass.renderTargetHeight = desc->height;
    pass.defaultRasterSampleCount =
        to_mtl_sample_count( desc->color_attachments[ 0 ]->sample_count );

    for ( u32 i = 0; i < desc->color_attachment_count; ++i )
    {
        FT_FROM_HANDLE( image, desc->color_attachments[ i ], MetalImage );

        if ( image->texture == nil )
        {
            render_pass->swapchain_render_pass = true;
        }

        pass.colorAttachments[ 0 ].texture = image->texture;
        pass.colorAttachments[ 0 ].loadAction =
            to_mtl_load_action( desc->color_attachment_load_ops[ i ] );
        pass.colorAttachments[ 0 ].storeAction = MTLStoreActionStore;

        render_pass->color_attachments[ i ] = image;
    }

    if ( desc->depth_stencil )
    {
        FT_FROM_HANDLE( image, desc->depth_stencil, MetalImage );
        pass.depthAttachment.texture = image->texture;
        pass.depthAttachment.loadAction =
            to_mtl_load_action( desc->depth_stencil_load_op );
        pass.depthAttachment.storeAction = MTLStoreActionStore;

        render_pass->interface.has_depth_stencil = true;
        render_pass->depth_attachment            = image;
    }
}

void
mtl_resize_render_pass( const Device*         idevice,
                        RenderPass*           irender_pass,
                        const RenderPassDesc* desc )
{
    FT_ASSERT( idevice );
    FT_ASSERT( irender_pass );
    FT_ASSERT( desc );

    FT_FROM_HANDLE( render_pass, irender_pass, MetalRenderPass );
    render_pass->render_pass.renderTargetWidth  = desc->width;
    render_pass->render_pass.renderTargetHeight = desc->height;
    render_pass->interface.width                = desc->width;
    render_pass->interface.height               = desc->height;
}

void
mtl_destroy_render_pass( const Device* idevice, RenderPass* irender_pass )
{
    FT_ASSERT( irender_pass );

    FT_FROM_HANDLE( render_pass, irender_pass, MetalRenderPass );

    [render_pass->render_pass release];
    operator delete( render_pass, std::nothrow );
}

void
mtl_create_shader( const Device* idevice, ShaderDesc* desc, Shader** p )
{
    FT_ASSERT( idevice );
    FT_ASSERT( desc );
    FT_ASSERT( p );

    FT_FROM_HANDLE( device, idevice, MetalDevice );

    FT_INIT_INTERNAL( shader, *p, MetalShader );

    auto create_function = []( const MetalDevice*      device,
                               MetalShader*            shader,
                               ShaderStage             stage,
                               const ShaderModuleDesc& desc )
    {
        dispatch_data_t lib_data =
            dispatch_data_create( desc.bytecode,
                                  desc.bytecode_size,
                                  nil,
                                  DISPATCH_DATA_DESTRUCTOR_DEFAULT );
        id<MTLLibrary> library = [device->device newLibraryWithData:lib_data
                                                              error:nil];

        shader->shaders[ static_cast<u32>( stage ) ] =
            [library newFunctionWithName:@"main0"];
    };

    create_function( device, shader, ShaderStage::eCompute, desc->compute );
    create_function( device, shader, ShaderStage::eVertex, desc->vertex );
    create_function( device,
                     shader,
                     ShaderStage::eTessellationControl,
                     desc->tessellation_control );
    create_function( device,
                     shader,
                     ShaderStage::eTessellationEvaluation,
                     desc->tessellation_evaluation );
    create_function( device, shader, ShaderStage::eFragment, desc->fragment );

    mtl_reflect( idevice, desc, &shader->interface );
}

void
mtl_destroy_shader( const Device* idevice, Shader* ishader )
{
    FT_ASSERT( idevice );
    FT_ASSERT( ishader );

    FT_FROM_HANDLE( shader, ishader, MetalShader );

    for ( u32 i = 0; i < static_cast<u32>( ShaderStage::eCount ); ++i )
    {
        if ( shader->shaders[ i ] )
        {
            [shader->shaders[ i ] release];
        }
    }
    operator delete( shader, std::nothrow );
}

void
mtl_create_descriptor_set_layout( const Device*         idevice,
                                  Shader*               ishader,
                                  DescriptorSetLayout** p )
{
    FT_INIT_INTERNAL( layout, *p, MetalDescriptorSetLayout );

    layout->interface.shader = ishader;
}

void
mtl_destroy_descriptor_set_layout( const Device*        idevice,
                                   DescriptorSetLayout* ilayout )
{
    FT_FROM_HANDLE( layout, ilayout, MetalDescriptorSetLayout );

    operator delete( layout, std::nothrow );
}

void
mtl_create_compute_pipeline( const Device*       idevice,
                             const PipelineDesc* desc,
                             Pipeline**          p )
{
}

void
mtl_create_graphics_pipeline( const Device*       idevice,
                              const PipelineDesc* desc,
                              Pipeline**          p )
{
    FT_ASSERT( idevice );
    FT_ASSERT( desc );
    FT_ASSERT( p );

    FT_FROM_HANDLE( device, idevice, MetalDevice );
    FT_FROM_HANDLE( shader, desc->shader, MetalShader );

    FT_INIT_INTERNAL( pipeline, *p, MetalPipeline );

    pipeline->pipeline_descriptor = [[MTLRenderPipelineDescriptor alloc] init];

    for ( u32 i = 0; i < static_cast<u32>( ShaderStage::eCount ); ++i )
    {
        if ( shader->shaders[ i ] == nil )
        {
            continue;
        }

        switch ( static_cast<ShaderStage>( i ) )
        {
        case ShaderStage::eVertex:
        {
            pipeline->pipeline_descriptor.vertexFunction = shader->shaders[ i ];
            break;
        }
        case ShaderStage::eFragment:
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

    MTLVertexDescriptor* vertex_layout = [MTLVertexDescriptor vertexDescriptor];

    for ( u32 i = 0; i < desc->vertex_layout.binding_desc_count; ++i )
    {
        u32 binding = desc->vertex_layout.binding_descs[ i ].binding;
        vertex_layout.layouts[ binding ].stepFunction =
            to_mtl_vertex_step_function(
                desc->vertex_layout.binding_descs[ i ].input_rate );
        vertex_layout.layouts[ binding ].stride =
            desc->vertex_layout.binding_descs[ i ].stride;
    }

    for ( u32 i = 0; i < desc->vertex_layout.attribute_desc_count; ++i )
    {
        u32 location = desc->vertex_layout.attribute_descs[ i ].location;
        vertex_layout.attributes[ location ].format = to_mtl_vertex_format(
            desc->vertex_layout.attribute_descs[ i ].format );
        vertex_layout.attributes[ location ].offset =
            desc->vertex_layout.attribute_descs[ i ].offset;
        vertex_layout.attributes[ location ].bufferIndex =
            desc->vertex_layout.attribute_descs[ i ].binding;
    }

    pipeline->pipeline_descriptor.vertexDescriptor = vertex_layout;

    FT_FROM_HANDLE( render_pass, desc->render_pass, MetalRenderPass );
    for ( u32 i = 0; i < render_pass->interface.color_attachment_count; ++i )
    {
        pipeline->pipeline_descriptor.colorAttachments[ i ].pixelFormat =
            to_mtl_format(
                render_pass->color_attachments[ i ]->interface.format );
    }

    if ( render_pass->interface.has_depth_stencil )
    {
        pipeline->pipeline_descriptor.depthAttachmentPixelFormat =
            to_mtl_format( render_pass->depth_attachment->interface.format );
    }

    NSError*                     err;
    MTLRenderPipelineReflection* reflection;
    pipeline->pipeline = [device->device
        newRenderPipelineStateWithDescriptor:pipeline->pipeline_descriptor
                                     options:MTLPipelineOptionArgumentInfo
                                  reflection:&reflection
                                       error:&err];
}

void
mtl_destroy_pipeline( const Device* idevice, Pipeline* ipipeline )
{
    FT_ASSERT( idevice );
    FT_ASSERT( ipipeline );

    FT_FROM_HANDLE( pipeline, ipipeline, MetalPipeline );

    [pipeline->pipeline release];
    [pipeline->pipeline_descriptor release];
    operator delete( pipeline, std::nothrow );
}

void
mtl_create_buffer( const Device* idevice, const BufferDesc* desc, Buffer** p )
{
    FT_ASSERT( idevice );
    FT_ASSERT( desc );
    FT_ASSERT( p );

    FT_FROM_HANDLE( device, idevice, MetalDevice );

    FT_INIT_INTERNAL( buffer, *p, MetalBuffer );

    buffer->interface.size            = desc->size;
    buffer->interface.descriptor_type = desc->descriptor_type;
    buffer->interface.memory_usage    = desc->memory_usage;
    buffer->interface.resource_state  = ResourceState::eUndefined;

    buffer->buffer = [device->device
        newBufferWithLength:desc->size
                    options:MTLResourceCPUCacheModeDefaultCache];
}

void
mtl_destroy_buffer( const Device* idevice, Buffer* ibuffer )
{
    FT_ASSERT( idevice );
    FT_ASSERT( ibuffer );

    FT_FROM_HANDLE( buffer, ibuffer, MetalBuffer );
    [buffer->buffer release];
    operator delete( buffer, std::nothrow );
}

void*
mtl_map_memory( const Device* idevice, Buffer* ibuffer )
{
    FT_FROM_HANDLE( buffer, ibuffer, MetalBuffer );
    buffer->interface.mapped_memory = buffer->buffer.contents;
    return buffer->interface.mapped_memory;
}

void
mtl_unmap_memory( const Device* idevice, Buffer* ibuffer )
{
    ibuffer->mapped_memory = nullptr;
}

void
mtl_create_sampler( const Device*      idevice,
                    const SamplerDesc* desc,
                    Sampler**          p )
{
    FT_ASSERT( idevice );
    FT_ASSERT( desc );
    FT_ASSERT( p );

    FT_FROM_HANDLE( device, idevice, MetalDevice );

    FT_INIT_INTERNAL( sampler, *p, MetalSampler );

    MTLSamplerDescriptor* sampler_descriptor =
        [[MTLSamplerDescriptor alloc] init];
    sampler_descriptor.minFilter =
        to_mtl_sampler_min_mag_filter( desc->min_filter );
    sampler_descriptor.magFilter =
        to_mtl_sampler_min_mag_filter( desc->mag_filter );
    sampler_descriptor.mipFilter =
        to_mtl_sampler_mip_filter( desc->mipmap_mode );
    sampler_descriptor.sAddressMode =
        to_mtl_sampler_address_mode( desc->address_mode_u );
    sampler_descriptor.tAddressMode =
        to_mtl_sampler_address_mode( desc->address_mode_v );
    sampler_descriptor.rAddressMode =
        to_mtl_sampler_address_mode( desc->address_mode_w );
    sampler_descriptor.maxAnisotropy = desc->max_anisotropy;
    sampler_descriptor.compareFunction =
        to_mtl_compare_function( desc->compare_op );
    sampler_descriptor.lodMinClamp = desc->min_lod;
    sampler_descriptor.lodMaxClamp = desc->max_lod;

    sampler->sampler =
        [device->device newSamplerStateWithDescriptor:sampler_descriptor];
}

void
mtl_destroy_sampler( const Device* idevice, Sampler* isampler )
{
    FT_ASSERT( idevice );
    FT_ASSERT( isampler );

    FT_FROM_HANDLE( sampler, isampler, MetalSampler );

    operator delete( sampler, std::nothrow );
}

void
mtl_create_image( const Device* idevice, const ImageDesc* desc, Image** p )
{
    FT_ASSERT( idevice );
    FT_ASSERT( desc );
    FT_ASSERT( p );

    FT_FROM_HANDLE( device, idevice, MetalDevice );

    FT_INIT_INTERNAL( image, *p, MetalImage );

    image->interface.width           = desc->width;
    image->interface.height          = desc->height;
    image->interface.depth           = desc->depth;
    image->interface.format          = desc->format;
    image->interface.descriptor_type = desc->descriptor_type;
    image->interface.sample_count    = desc->sample_count;
    image->interface.layer_count     = desc->layer_count;
    image->interface.mip_level_count = desc->mip_levels;

    image->texture_descriptor = [[MTLTextureDescriptor alloc] init];

    image->texture_descriptor.width            = desc->width;
    image->texture_descriptor.height           = desc->height;
    image->texture_descriptor.depth            = desc->depth;
    image->texture_descriptor.arrayLength      = desc->layer_count;
    image->texture_descriptor.mipmapLevelCount = desc->mip_levels;
    image->texture_descriptor.sampleCount =
        to_mtl_sample_count( desc->sample_count );
    image->texture_descriptor.storageMode = MTLStorageModeManaged; // TODO:
    image->texture_descriptor.textureType = MTLTextureType2D;      // TODO:
    image->texture_descriptor.usage =
        to_mtl_texture_usage( desc->descriptor_type );
    image->texture_descriptor.pixelFormat = to_mtl_format( desc->format );

    image->texture =
        [device->device newTextureWithDescriptor:image->texture_descriptor];
}

void
mtl_destroy_image( const Device* idevice, Image* iimage )
{
    FT_ASSERT( idevice );
    FT_ASSERT( iimage );

    FT_FROM_HANDLE( image, iimage, MetalImage );

    [image->texture_descriptor release];
    [image->texture release];
    operator delete( image, std::nothrow );
}

static inline void
count_binding_types( MetalShader*                      shader,
                     std::vector<MetalSamplerBinding>& sampler_bindings,
                     std::vector<MetalImageBinding>&   image_bindings,
                     std::vector<MetalBufferBinding>&  buffer_bindings )
{
    for ( u32 i = 0; i < static_cast<u32>( ShaderStage::eCount ); ++i )
    {
        if ( shader->shaders[ i ] == nil )
        {
            continue;
        }

        ShaderStage stage = static_cast<ShaderStage>( i );

        for ( u32 b = 0; b < shader->interface.reflect_data[ i ].binding_count;
              ++b )
        {
            auto& binding = shader->interface.reflect_data[ i ].bindings[ b ];
            switch ( binding.descriptor_type )
            {
            case DescriptorType::eSampler:
            {
                auto& b   = sampler_bindings.emplace_back();
                b         = {};
                b.stage   = stage;
                b.binding = binding.binding;
                break;
            }
            case DescriptorType::eSampledImage:
            {
                auto& b   = image_bindings.emplace_back();
                b         = {};
                b.stage   = stage;
                b.binding = binding.binding;

                break;
            }
            case DescriptorType::eUniformBuffer:
            {
                auto& b   = buffer_bindings.emplace_back();
                b         = {};
                b.stage   = stage;
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
}

void
mtl_create_descriptor_set( const Device*            idevice,
                           const DescriptorSetDesc* desc,
                           DescriptorSet**          p )
{
    FT_ASSERT( idevice );
    FT_ASSERT( desc );
    FT_ASSERT( p );

    FT_FROM_HANDLE( shader, desc->descriptor_set_layout->shader, MetalShader );

    FT_INIT_INTERNAL( set, *p, MetalDescriptorSet );

    std::vector<MetalSamplerBinding> sampler_bindings;
    std::vector<MetalImageBinding>   image_bindings;
    std::vector<MetalBufferBinding>  buffer_bindings;

    count_binding_types( shader,
                         sampler_bindings,
                         image_bindings,
                         buffer_bindings );

    set->sampler_binding_count = sampler_bindings.size();
    set->buffer_binding_count  = buffer_bindings.size();
    set->image_binding_count   = image_bindings.size();

    if ( set->sampler_binding_count > 0 )
    {
        set->sampler_bindings = new ( std::nothrow )
            MetalSamplerBinding[ set->sampler_binding_count ];
        std::copy( sampler_bindings.begin(),
                   sampler_bindings.end(),
                   set->sampler_bindings );
    }

    if ( set->image_binding_count > 0 )
    {
        set->image_bindings =
            new ( std::nothrow ) MetalImageBinding[ set->image_binding_count ];
        std::copy( image_bindings.begin(),
                   image_bindings.end(),
                   set->image_bindings );
    }

    if ( set->buffer_binding_count > 0 )
    {
        set->buffer_bindings = new ( std::nothrow )
            MetalBufferBinding[ set->buffer_binding_count ];
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
        operator delete( set->sampler_bindings, std::nothrow );
    }

    if ( set->image_bindings )
    {
        operator delete( set->image_bindings, std::nothrow );
    }

    if ( set->buffer_bindings )
    {
        operator delete( set->buffer_bindings, std::nothrow );
    }

    operator delete( set, std::nothrow );
}

void
mtl_update_descriptor_set( const Device*          idevice,
                           DescriptorSet*         iset,
                           u32                    count,
                           const DescriptorWrite* writes )
{
    FT_FROM_HANDLE( set, iset, MetalDescriptorSet );

    for ( u32 i = 0; i < count; ++i )
    {
        u32 binding = writes[ i ].binding;

        switch ( writes[ i ].descriptor_type )
        {
        case DescriptorType::eSampler:
        {
            auto it = std::find_if(
                set->sampler_bindings,
                set->sampler_bindings + set->sampler_binding_count,
                [ binding ]( MetalSamplerBinding& sampler_binding )
                { return sampler_binding.binding == binding; } );

            if ( it != set->sampler_bindings + set->sampler_binding_count )
            {
                FT_FROM_HANDLE( sampler,
                                writes[ i ].sampler_descriptors[ 0 ].sampler,
                                MetalSampler );
                it->sampler = sampler->sampler;
            }
            break;
        }
        case DescriptorType::eSampledImage:
        {
            auto it =
                std::find_if( set->image_bindings,
                              set->image_bindings + set->image_binding_count,
                              [ binding ]( MetalImageBinding& image_binding )
                              { return image_binding.binding == binding; } );

            if ( it != set->image_bindings + set->image_binding_count )
            {
                FT_FROM_HANDLE( image,
                                writes[ i ].image_descriptors[ 0 ].image,
                                MetalImage );
                it->image = image->texture;
            }
            break;
        }
        case DescriptorType::eUniformBuffer:
        {
            auto it =
                std::find_if( set->buffer_bindings,
                              set->buffer_bindings + set->buffer_binding_count,
                              [ binding ]( MetalBufferBinding& buffer_binding )
                              { return buffer_binding.binding == binding; } );

            if ( it != set->buffer_bindings + set->buffer_binding_count )
            {
                FT_FROM_HANDLE( buffer,
                                writes[ i ].buffer_descriptors[ 0 ].buffer,
                                MetalBuffer );
                it->buffer = buffer->buffer;
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

void
mtl_create_ui_context( CommandBuffer* cmd, const UiDesc* desc, UiContext** p )
{
    FT_ASSERT( p );

    FT_FROM_HANDLE( device, desc->device, MetalDevice );

    auto context              = new ( std::nothrow ) MetalUiContext {};
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

    ImGui_ImplSDL2_InitForMetal(
        static_cast<SDL_Window*>( desc->window->handle ) );
    ImGui_ImplMetal_Init( device->device );
}

void
mtl_destroy_ui_context( const Device* idevice, UiContext* icontext )
{
    FT_ASSERT( icontext );

    FT_FROM_HANDLE( context, icontext, MetalUiContext );

    ImGui_ImplMetal_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    operator delete( context, std::nothrow );
}

void
mtl_ui_begin_frame( UiContext*, CommandBuffer* icmd )
{
    FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
    ImGui_ImplMetal_NewFrame( cmd->pass_descriptor );
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void
mtl_ui_end_frame( UiContext*, CommandBuffer* icmd )
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

void
mtl_cmd_begin_render_pass( const CommandBuffer*       icmd,
                           const RenderPassBeginDesc* desc )
{
    FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
    FT_FROM_HANDLE( render_pass, desc->render_pass, MetalRenderPass );

    if ( render_pass->swapchain_render_pass )
    {
        render_pass->render_pass.colorAttachments[ 0 ].texture =
            render_pass->color_attachments[ 0 ]->texture;
    }

    for ( u32 i = 0; i < render_pass->interface.color_attachment_count; ++i )
    {
        render_pass->render_pass.colorAttachments[ i ].clearColor =
            MTLClearColorMake( desc->clear_values[ i ].color[ 0 ],
                               desc->clear_values[ i ].color[ 1 ],
                               desc->clear_values[ i ].color[ 2 ],
                               desc->clear_values[ i ].color[ 3 ] );
    }

    if ( render_pass->interface.has_depth_stencil )
    {
        render_pass->render_pass.depthAttachment.clearDepth =
            desc->clear_values[ render_pass->interface.color_attachment_count ]
                .depth;
    }

    cmd->encoder =
        [cmd->cmd renderCommandEncoderWithDescriptor:render_pass->render_pass];
    cmd->pass_descriptor = render_pass->render_pass;
}

void
mtl_cmd_end_render_pass( const CommandBuffer* icmd )
{
    FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );

    [cmd->encoder endEncoding];
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

    [cmd->encoder setRenderPipelineState:pipeline->pipeline];
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

    [cmd->encoder drawPrimitives:MTLPrimitiveTypeTriangle
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

    [cmd->encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                             indexCount:index_count
                              indexType:MTLIndexTypeUInt32
                            indexBuffer:cmd->index_buffer
                      indexBufferOffset:first_index];
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

    [cmd->encoder setVertexBuffer:buffer->buffer offset:offset atIndex:0];
}

void
mtl_cmd_bind_index_buffer_u16( const CommandBuffer* icmd,
                               const Buffer*        ibuffer,
                               const u64            offset )
{
    FT_ASSERT( icmd );
    FT_ASSERT( ibuffer );

    FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
    FT_FROM_HANDLE( buffer, ibuffer, MetalBuffer );

    cmd->index_type   = MTLIndexTypeUInt16;
    cmd->index_buffer = buffer->buffer;
}

void
mtl_cmd_bind_index_buffer_u32( const CommandBuffer* icmd,
                               const Buffer*        ibuffer,
                               u64                  offset )
{
    FT_ASSERT( icmd );
    FT_ASSERT( ibuffer );

    FT_FROM_HANDLE( cmd, icmd, MetalCommandBuffer );
    FT_FROM_HANDLE( buffer, ibuffer, MetalBuffer );

    cmd->index_type   = MTLIndexTypeUInt32;
    cmd->index_buffer = buffer->buffer;
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
        case ShaderStage::eVertex:
        {
            [cmd->encoder
                setVertexSamplerState:set->sampler_bindings[ i ].sampler
                              atIndex:set->sampler_bindings[ i ].binding];
            break;
        }
        case ShaderStage::eFragment:
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
        case ShaderStage::eVertex:
        {
            [cmd->encoder setVertexTexture:set->image_bindings[ i ].image
                                   atIndex:set->image_bindings[ i ].binding];
            break;
        }
        case ShaderStage::eFragment:
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
        case ShaderStage::eVertex:
        {
            [cmd->encoder setVertexBuffer:set->buffer_bindings[ i ].buffer
                                   offset:0
                                  atIndex:set->buffer_bindings[ i ].binding];
            break;
        }
        case ShaderStage::eFragment:
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
mtl_create_renderer_backend( const RendererBackendDesc*, RendererBackend** p )
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
    create_render_pass            = mtl_create_render_pass;
    resize_render_pass            = mtl_resize_render_pass;
    destroy_render_pass           = mtl_destroy_render_pass;
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
    create_ui_context             = mtl_create_ui_context;
    destroy_ui_context            = mtl_destroy_ui_context;
    ui_begin_frame                = mtl_ui_begin_frame;
    ui_end_frame                  = mtl_ui_end_frame;
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
