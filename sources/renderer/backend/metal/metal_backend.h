#pragma once

#ifdef METAL_BACKEND

#ifdef METAL_BACKEND_INCLUDE_OBJC
#import <Metal/Metal.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#endif
#include "../renderer_backend.h"

struct MetalRendererBackend
{
	void*                  window;
	struct RendererBackend interface;
};

struct MetalDevice
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLDevice> device;
#else
	void* device;
#endif
	void*         view;
	struct Device interface;
};

struct MetalCommandPool
{
	struct CommandPool interface;
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
	struct CommandBuffer interface;
};

struct MetalQueue
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLCommandQueue> queue;
#else
	void* queue;
#endif
	struct Queue interface;
};

struct MetalSemaphore
{
	struct Semaphore interface;
};

struct MetalFence
{
	struct Fence interface;
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
	struct Sampler interface;
};

struct MetalImage
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLTexture> texture;
#else
	void* texture;
#endif
	struct Image interface;
};

struct MetalBuffer
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLBuffer> buffer;
#else
	void* buffer;
#endif
	struct Buffer interface;
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
	u32              current_image_index;
	struct Swapchain interface;
};

struct MetalRenderPass
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
	MTLRenderPassDescriptor* render_pass;
#else
	void* render_pass;
#endif
	b32                swapchain_render_pass;
	struct MetalImage* color_attachments[ MAX_ATTACHMENTS_COUNT ];
	struct MetalImage* depth_attachment;
};

struct MetalShader
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLFunction> shaders[ FT_SHADER_STAGE_COUNT ];
#else
	void* shaders[ FT_SHADER_STAGE_COUNT ];
#endif
	struct Shader interface;
};

struct MetalDescriptorSetLayout
{
	struct DescriptorSetLayout interface;
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
	struct Pipeline interface;
};

struct MetalSamplerBinding
{
	enum ShaderStage stage;
	u32              binding;
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLSamplerState> sampler;
#else
	void* sampler;
#endif
};

struct MetalImageBinding
{
	enum ShaderStage stage;
	u32              binding;
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLTexture> image;
#else
	void* image;
#endif
};

struct MetalBufferBinding
{
	enum ShaderStage stage;
	u32              binding;
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLBuffer> buffer;
#else
	void* buffer;
#endif
};

struct MetalDescriptorSet
{
	u32                         sampler_binding_count;
	struct MetalSamplerBinding* sampler_bindings;
	u32                         image_binding_count;
	struct MetalImageBinding*   image_bindings;
	u32                         buffer_binding_count;
	struct MetalBufferBinding*  buffer_bindings;
	struct DescriptorSet        interface;
};

void
mtl_create_renderer_backend( const struct RendererBackendInfo*,
                             struct RendererBackend** p );

#endif
