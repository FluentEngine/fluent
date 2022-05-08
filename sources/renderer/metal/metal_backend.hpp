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
	id<MTLTexture> texture;
#else
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
	id<MTLFunction> shaders[ static_cast<u32>( ShaderStage::COUNT ) ];
#else
	void* shaders[ static_cast<u32>( ShaderStage::COUNT ) ];
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
	id<MTLDepthStencilState>     depth_stencil_state;
	MTLPrimitiveType             primitive_type;
#else
	void* pipeline_descriptor;
	void* pipeline;
	void* depth_stencil_state;
	u32   primitive_type;
#endif
	Pipeline interface;
};

struct MetalSamplerBinding
{
	ShaderStage stage;
	u32         binding;
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLSamplerState> sampler;
#else
	void* sampler;
#endif
};

struct MetalImageBinding
{
	ShaderStage stage;
	u32         binding;
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLTexture> image;
#else
	void* image;
#endif
};

struct MetalBufferBinding
{
	ShaderStage stage;
	u32         binding;
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLBuffer> buffer;
#else
	void* buffer;
#endif
};

struct MetalDescriptorSet
{
	u32                  sampler_binding_count;
	MetalSamplerBinding* sampler_bindings;
	u32                  image_binding_count;
	MetalImageBinding*   image_bindings;
	u32                  buffer_binding_count;
	MetalBufferBinding*  buffer_bindings;
	DescriptorSet        interface;
};

struct MetalUiContext
{
	UiContext interface;
};

void
mtl_create_renderer_backend( const RendererBackendInfo*, RendererBackend** p );

} // namespace fluent

#endif
