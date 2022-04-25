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
    FT_FROM_HANDLE( queue, desc->queue, MetalQueue );

    auto swapchain              = new ( std::nothrow ) MetalSwapchain {};
    swapchain->interface.handle = swapchain;
    *p                          = &swapchain->interface;

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
    
    swapchain->interface.image_count = swapchain->swapchain.maximumDrawableCount;
    
    swapchain->interface.images =
        new ( std::nothrow ) Image*[ swapchain->interface.image_count ];

    for ( u32 i = 0; i < swapchain->interface.image_count; i++ )
    {
        auto image                       = new ( std::nothrow ) MetalImage {};
        image->interface.handle          = image;
        image->interface.width           = desc->width;
        image->interface.height          = desc->height;
        image->interface.depth           = 1;
        image->interface.format          = desc->format;
        image->interface.layer_count     = 1;
        image->interface.descriptor_type = DescriptorType::eColorAttachment;
        image->interface.mip_level_count = 1;
        image->interface.sample_count    = SampleCount::e1;
        swapchain->interface.images[ i ] = &image->interface;
    }
}

void
mtl_resize_swapchain( const Device* idevice,
                      Swapchain*    iswapchain,
                      u32           width,
                      u32           height )
{
}

void
mtl_destroy_swapchain( const Device* idevice, Swapchain* iswapchain )
{
    FT_ASSERT( iswapchain );

    FT_FROM_HANDLE( device, idevice, MetalDevice );
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

    auto cmd_pool              = new ( std::nothrow ) MetalCommandPool {};
    cmd_pool->interface.handle = cmd_pool;
    *p                         = &cmd_pool->interface;

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
mtl_create_shader( const Device* idevice, ShaderDesc* desc, Shader** p )
{
    FT_ASSERT( idevice );
    FT_ASSERT( desc );
    FT_ASSERT( p );

    FT_FROM_HANDLE( device, idevice, MetalDevice );

    auto shader              = new ( std::nothrow ) MetalShader {};
    shader->interface.handle = shader;
    *p                       = &shader->interface;

    NSError* err;

    dispatch_data_t lib_data = dispatch_data_create( desc->bytecode,
                                                     desc->bytecode_size,
                                                     dispatch_get_main_queue(),
                                                     ^ {} );
    id<MTLLibrary>  library  = [device->device newLibraryWithData:lib_data
                                                          error:&err];

    shader->shader = [library newFunctionWithName:@"main0"];
}

void
mtl_destroy_shader( const Device* idevice, Shader* ishader )
{
    FT_ASSERT( idevice );
    FT_ASSERT( ishader );

    FT_FROM_HANDLE( shader, ishader, MetalShader );
    [shader->shader release];
    operator delete( shader, std::nothrow );
}

void
mtl_create_descriptor_set_layout( const Device*         idevice,
                                  u32                   shader_count,
                                  Shader**              ishaders,
                                  DescriptorSetLayout** p )
{
}

void
mtl_destroy_descriptor_set_layout( const Device*        idevice,
                                   DescriptorSetLayout* ilayout )
{
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
}

void
mtl_destroy_pipeline( const Device* idevice, Pipeline* ipipeline )
{
}

void
mtl_create_buffer( const Device* idevice, const BufferDesc* desc, Buffer** p )
{
}

void
mtl_destroy_buffer( const Device* idevice, Buffer* ibuffer )
{
}

void*
mtl_map_memory( const Device* idevice, Buffer* ibuffer )
{
    return nullptr;
}

void
mtl_unmap_memory( const Device* idevice, Buffer* ibuffer )
{
}

void
mtl_create_sampler( const Device*      idevice,
                    const SamplerDesc* desc,
                    Sampler**          p )
{
}

void
mtl_destroy_sampler( const Device* idevice, Sampler* isampler )
{
}

void
mtl_create_image( const Device* idevice, const ImageDesc* desc, Image** p )
{
}

void
mtl_destroy_image( const Device* idevice, Image* iimage )
{
}

void
mtl_create_descriptor_set( const Device*            idevice,
                           const DescriptorSetDesc* desc,
                           DescriptorSet**          p )
{
}

void
mtl_destroy_descriptor_set( const Device* idevice, DescriptorSet* iset )
{
}

void
mtl_update_descriptor_set( const Device*          idevice,
                           DescriptorSet*         iset,
                           u32                    count,
                           const DescriptorWrite* writes )
{
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
mtl_cmd_begin_render_pass( const CommandBuffer*  icmd,
                           const RenderPassBeginDesc* desc )
{

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
                                                y + height,
                                                width,
                                                -height,
                                                min_depth,
                                                max_depth }];
}

void
mtl_cmd_bind_pipeline( const CommandBuffer* icmd, const Pipeline* ipipeline )
{
}

void
mtl_cmd_draw( const CommandBuffer* icmd,
              u32                  vertex_count,
              u32                  instance_count,
              u32                  first_vertex,
              u32                  first_instance )
{
}

void
mtl_cmd_draw_indexed( const CommandBuffer* icmd,
                      u32                  index_count,
                      u32                  instance_count,
                      u32                  first_index,
                      i32                  vertex_offset,
                      u32                  first_instance )
{
}

void
mtl_cmd_bind_vertex_buffer( const CommandBuffer* icmd,
                            const Buffer*        ibuffer,
                            const u64            offset )
{
}

void
mtl_cmd_bind_index_buffer_u16( const CommandBuffer* icmd,
                               const Buffer*        ibuffer,
                               const u64            offset )
{
}

void
mtl_cmd_bind_index_buffer_u32( const CommandBuffer* icmd,
                               const Buffer*        ibuffer,
                               u64                  offset )
{
}

void
mtl_cmd_copy_buffer( const CommandBuffer* icmd,
                     const Buffer*        isrc,
                     u64                  src_offset,
                     Buffer*              idst,
                     u64                  dst_offset,
                     u64                  size )
{
}

void
mtl_cmd_copy_buffer_to_image( const CommandBuffer* icmd,
                              const Buffer*        isrc,
                              u64                  src_offset,
                              Image*               idst )
{
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

    auto renderer_backend = new ( std::nothrow ) MetalRendererBackend {};
    renderer_backend->interface.handle = renderer_backend;
    *p                                 = &renderer_backend->interface;
}

}
#endif
