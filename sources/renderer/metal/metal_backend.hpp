#pragma once

#ifdef METAL_BACKEND

#ifdef METAL_BACKEND_INCLUDE_OBJC
#import <Metal/Metal.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#endif
#include "renderer/renderer_backend.hpp"

namespace fluent
{

struct MetalRendererBackend
{
    RendererBackend interface;
};

struct MetalDevice
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
    id<MTLDevice> device;
#else
    void* device;
#endif
    void*  view;
    Device interface;
};

struct MetalCommandPool
{
    CommandPool interface;
};

struct MetalCommandBuffer
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
    id<MTLCommandBuffer>        cmd;
    id<MTLRenderCommandEncoder> encoder;
    MTLRenderPassDescriptor*    pass_descriptor;
    id<MTLBuffer>               index_buffer;
    MTLIndexType                index_type;
    MTLPrimitiveType            primitive_type;
#else
    void* cmd;
    void* encoder;
    void* pass_descriptor;
    void* index_buffer;
    u32   index_type;
    u32   primitive_type;
#endif
    CommandBuffer interface;
};

struct MetalQueue
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
    id<MTLCommandQueue> queue;
#else
    void* queue;
#endif
    Queue interface;
};

struct MetalSemaphore
{
    Semaphore interface;
};

struct MetalFence
{
    Fence interface;
};

struct MetalSampler
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
    MTLSamplerDescriptor* sampler_descriptor;
    id<MTLSamplerState>   sampler;
#else
    void* sampler_descriptor;
    void* sampler;
#endif
    Sampler interface;
};

struct MetalImage
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
    MTLTextureDescriptor* texture_descriptor;
    id<MTLTexture>        texture;
#else
    void* texture_descriptor;
    void* texture;
#endif
    Image interface;
};

struct MetalBuffer
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
    id<MTLBuffer> buffer;
#else
    void* buffer;
#endif
    Buffer interface;
};

struct MetalSwapchain
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
    CAMetalLayer*       swapchain;
    id<CAMetalDrawable> drawable;
#else
    void* swapchain;
    void* drawable;
#endif
    u32       current_image_index;
    Swapchain interface;
};

struct MetalRenderPass
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
    MTLRenderPassDescriptor* render_pass;
#else
    void* render_pass;
#endif
    b32         swapchain_render_pass;
    MetalImage* color_attachments[ MAX_ATTACHMENTS_COUNT ];
    MetalImage* depth_attachment;
    RenderPass  interface;
};

struct MetalShader
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
    id<MTLFunction> shaders[ static_cast<u32>( ShaderStage::eCount ) ];
#else
    void* shaders[ static_cast<u32>( ShaderStage::eCount ) ];
#endif
    Shader interface;
};

struct MetalDescriptorSetLayout
{
    DescriptorSetLayout interface;
};

struct MetalPipeline
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
    MTLRenderPipelineDescriptor* pipeline_descriptor;
    id<MTLRenderPipelineState>   pipeline;
#else
    void* pipeline_descriptor;
    void* pipeline;
#endif
    Pipeline interface;
};

struct MetalStageDescriptors
{
    u32 sampler_count;
    u32 texture_count;
    u32 buffer_count;
#ifdef METAL_BACKEND_INCLUDE_OBJC
    id<MTLSamplerState>* samplers;
    id<MTLTexture>*      textures;
    id<MTLBuffer>*       buffers;
#else
    void* samplers;
    void* textures;
    void* buffers;
#endif
};

struct MetalDescriptorSet
{
    MetalStageDescriptors
                  descriptors[ static_cast<u32>( ShaderStage::eCount ) ];
    DescriptorSet interface;
};

struct MetalUiContext
{
    UiContext interface;
};

void
mtl_create_renderer_backend( const RendererBackendDesc*, RendererBackend** p );

} // namespace fluent

#endif
