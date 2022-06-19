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
	void*                      window;
	struct ft_renderer_backend interface;
};

struct MetalDevice
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLDevice> device;
#else
	void*    device;
#endif
	void*            view;
	struct ft_device interface;
};

struct MetalCommandPool
{
	struct ft_command_pool interface;
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
	void*    cmd;
	void*    encoder;
	void*    pass_descriptor;
	void*    index_buffer;
	uint32_t index_type;
	uint32_t primitive_type;
#endif
	struct ft_command_buffer interface;
};

struct MetalQueue
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLCommandQueue> queue;
#else
	void*    queue;
#endif
	struct ft_queue interface;
};

struct MetalSemaphore
{
	struct ft_semaphore interface;
};

struct MetalFence
{
	struct ft_fence interface;
};

struct MetalSampler
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
	MTLSamplerDescriptor* sampler_descriptor;
	id<MTLSamplerState>   sampler;
#else
	void*    sampler_descriptor;
	void*    sampler;
#endif
	struct ft_sampler interface;
};

struct MetalImage
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLTexture> texture;
#else
	void*    texture;
#endif
	struct ft_image interface;
};

struct MetalBuffer
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLBuffer> buffer;
#else
	void*    buffer;
#endif
	struct ft_buffer interface;
};

struct MetalSwapchain
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
	CAMetalLayer*       swapchain;
	id<CAMetalDrawable> drawable;
#else
	void*    swapchain;
	void*    drawable;
#endif
	uint32_t            current_image_index;
	struct ft_swapchain interface;
};

struct MetalRenderPass
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
	MTLRenderPassDescriptor* render_pass;
#else
	void*    render_pass;
#endif
	bool               swapchain_render_pass;
	struct MetalImage* color_attachments[ FT_MAX_ATTACHMENTS_COUNT ];
	struct MetalImage* depth_attachment;
};

struct MetalShader
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLFunction> shaders[ FT_SHADER_STAGE_COUNT ];
#else
	void*    shaders[ FT_SHADER_STAGE_COUNT ];
#endif
	struct ft_shader interface;
};

struct MetalDescriptorSetLayout
{
	struct ft_descriptor_set_layout interface;
};

struct MetalPipeline
{
#ifdef METAL_BACKEND_INCLUDE_OBJC
	MTLRenderPipelineDescriptor* pipeline_descriptor;
	id<MTLRenderPipelineState>   pipeline;
	id<MTLDepthStencilState>     depth_stencil_state;
	MTLPrimitiveType             primitive_type;
#else
	void*    pipeline_descriptor;
	void*    pipeline;
	void*    depth_stencil_state;
	uint32_t primitive_type;
#endif
	struct ft_pipeline interface;
};

struct MetalSamplerBinding
{
	enum ft_shader_stage stage;
	uint32_t             binding;
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLSamplerState> sampler;
#else
	void*    sampler;
#endif
};

struct MetalImageBinding
{
	enum ft_shader_stage stage;
	uint32_t             binding;
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLTexture> image;
#else
	void*    image;
#endif
};

struct MetalBufferBinding
{
	enum ft_shader_stage stage;
	uint32_t             binding;
#ifdef METAL_BACKEND_INCLUDE_OBJC
	id<MTLBuffer> buffer;
#else
	void*    buffer;
#endif
};

struct MetalDescriptorSet
{
	uint32_t                    sampler_binding_count;
	struct MetalSamplerBinding* sampler_bindings;
	uint32_t                    image_binding_count;
	struct MetalImageBinding*   image_bindings;
	uint32_t                    buffer_binding_count;
	struct MetalBufferBinding*  buffer_bindings;
	struct ft_descriptor_set    interface;
};

void
mtl_create_renderer_backend( const struct ft_renderer_backend_info*,
                             struct ft_renderer_backend** p );

#endif
