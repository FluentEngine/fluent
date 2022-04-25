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
#endif
    CommandBuffer interface;
};

struct MetalQueue
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
    id<MTLCommandQueue> queue;
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
    Sampler interface;
};

struct MetalImage
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
    id<MTLTexture> texture;
#endif
    Image interface;
};

struct MetalBuffer
{
    Buffer interface;
};

struct MetalSwapchain
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
    CAMetalLayer*       swapchain;
    id<CAMetalDrawable> drawable;
#endif
    u32       current_image_index;
    Swapchain interface;
};

struct MetalRenderPass
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
    MTLRenderPassDescriptor* render_pass;
#endif
};

struct MetalShader
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
    id<MTLFunction> shader;
#endif
    Shader interface;
};

struct MetalDescriptorSetLayout
{
    DescriptorSetLayout interface;
};

struct MetalPipeline
{
    Pipeline interface;
};

struct MetalDescriptorSet
{
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
