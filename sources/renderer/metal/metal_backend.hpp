#pragma once

#ifdef METAL_BACKEND
#include "renderer/renderer_backend.hpp"

namespace fluent
{

struct MetalRendererBackend
{
    RendererBackend interface;
};

struct MetalDevice
{
    void*  device;
    void*  view;
    Device interface;
};

struct MetalCommandPool
{
    CommandPool interface;
};

struct MetalCommandBuffer
{
    void*         cmd;
    void*         encoder;
    CommandBuffer interface;
};

struct MetalQueue
{
    void* queue;
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
    void* texture;
    Image interface;
};

struct MetalBuffer
{
    Buffer interface;
};

struct MetalSwapchain
{
    void*     swapchain;
    u32       current_image_index;
    Swapchain interface;
};

struct MetalRenderPass
{
    void*      render_pass;
    RenderPass interface;
};

struct MetalShader
{
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
