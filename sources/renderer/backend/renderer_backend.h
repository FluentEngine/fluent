#pragma once

#include "base/base.h"

#include "renderer/backend/renderer_enums.h"
#include "renderer/shader_reflection/shader_reflection.h"

#undef MemoryBarrier

#define MAX_DEVICE_COUNT                    1
#define MAX_ATTACHMENTS_COUNT               10
#define MAX_PUSH_CONSTANT_RANGE             128
#define MAX_VERTEX_BINDING_COUNT            15
#define MAX_VERTEX_ATTRIBUTE_COUNT          15
#define MAX_DESCRIPTOR_BINDING_COUNT        15
#define MAX_SET_COUNT                       10
#define RESOURCE_LOADER_STAGING_BUFFER_SIZE 25 * 1024 * 1024 * 8

#ifdef __cplusplus
extern "C"
{
#endif

	struct WsiInfo;

	// Forward declares
	struct Window;
	struct Buffer;
	struct StagingBuffer;
	struct Image;
	struct Queue;
	struct CommandBuffer;
	struct CommandPool;

	struct RendererBackendInfo
	{
		enum RendererAPI api;
		struct WsiInfo*  wsi_info;
	};

	struct RendererBackend
	{
		enum RendererAPI api;
		ft_handle        handle;
	};

	struct DeviceInfo
	{
		struct RendererBackend* backend;
	};

	struct Device
	{
		enum RendererAPI api;
		ft_handle        handle;
	};

	struct CommandPoolInfo
	{
		struct Queue* queue;
	};

	struct CommandPool
	{
		struct Queue* queue;
		ft_handle     handle;
	};

	struct CommandBuffer
	{
		struct Queue* queue;
		ft_handle     handle;
	};

	struct QueueInfo
	{
		enum QueueType queue_type;
	};

	struct Queue
	{
		u32            family_index;
		enum QueueType type;
		ft_handle      handle;
	};

	struct Semaphore
	{
		ft_handle handle;
	};

	struct Fence
	{
		ft_handle handle;
	};

	struct SamplerInfo
	{
		enum Filter             mag_filter;
		enum Filter             min_filter;
		enum SamplerMipmapMode  mipmap_mode;
		enum SamplerAddressMode address_mode_u;
		enum SamplerAddressMode address_mode_v;
		enum SamplerAddressMode address_mode_w;
		f32                     mip_lod_bias;
		b32                     anisotropy_enable;
		f32                     max_anisotropy;
		b32                     compare_enable;
		enum CompareOp          compare_op;
		f32                     min_lod;
		f32                     max_lod;
	};

	struct Sampler
	{
		ft_handle handle;
	};

	struct ImageInfo
	{
		u32                 width;
		u32                 height;
		u32                 depth;
		enum Format         format;
		u32                 sample_count;
		u32                 layer_count;
		u32                 mip_levels;
		enum DescriptorType descriptor_type;
	};

	struct Image
	{
		u32                 width;
		u32                 height;
		u32                 depth;
		enum Format         format;
		u32                 sample_count;
		u32                 mip_level_count;
		u32                 layer_count;
		enum DescriptorType descriptor_type;
		ft_handle           handle;
	};

	struct BufferInfo
	{
		u64                 size;
		enum DescriptorType descriptor_type;
		enum MemoryUsage    memory_usage;
	};

	struct Buffer
	{
		u64                 size;
		enum ResourceState  resource_state;
		enum DescriptorType descriptor_type;
		enum MemoryUsage    memory_usage;
		void*               mapped_memory;
		ft_handle           handle;
	};

	struct SwapchainInfo
	{
		struct Queue*   queue;
		u32             width;
		u32             height;
		enum Format     format;
		b32             vsync;
		u32             min_image_count;
		struct WsiInfo* wsi_info;
	};

	struct Swapchain
	{
		u32            min_image_count;
		u32            image_count;
		u32            width;
		u32            height;
		enum Format    format;
		struct Image** images;
		struct Queue*  queue;
		b32            vsync;
		ft_handle      handle;
	};

	struct QueueSubmitInfo
	{
		u32                    wait_semaphore_count;
		struct Semaphore**     wait_semaphores;
		u32                    command_buffer_count;
		struct CommandBuffer** command_buffers;
		u32                    signal_semaphore_count;
		struct Semaphore**     signal_semaphores;
		struct Fence*          signal_fence;
	};

	struct QueuePresentInfo
	{
		u32                wait_semaphore_count;
		struct Semaphore** wait_semaphores;
		struct Swapchain*  swapchain;
		u32                image_index;
	};

	typedef f32 ColorClearValue[ 4 ];

	struct DepthStencilClearValue
	{
		f32 depth;
		u32 stencil;
	};

	struct ClearValue
	{
		ColorClearValue               color;
		struct DepthStencilClearValue depth_stencil;
	};

	struct AttachmentInfo
	{
		const struct Image*   image;
		enum AttachmentLoadOp load_op;
		struct ClearValue     clear_value;
	};

	struct RenderPassBeginInfo
	{
		u32                   width;
		u32                   height;
		u32                   color_attachment_count;
		struct AttachmentInfo color_attachments[ MAX_ATTACHMENTS_COUNT ];
		struct AttachmentInfo depth_attachment;
	};

	// TODO:
	struct MemoryBarrier
	{
		void* allocation;
	};

	struct BufferBarrier
	{
		enum ResourceState old_state;
		enum ResourceState new_state;
		struct Buffer*     buffer;
		struct Queue*      src_queue;
		struct Queue*      dst_queue;
		u32                size;
		u32                offset;
	};

	struct ImageBarrier
	{
		enum ResourceState  old_state;
		enum ResourceState  new_state;
		const struct Image* image;
		const struct Queue* src_queue;
		const struct Queue* dst_queue;
	};

	struct ShaderModuleInfo
	{
		u32         bytecode_size;
		const void* bytecode;
	};

	struct ShaderInfo
	{
		struct ShaderModuleInfo compute;
		struct ShaderModuleInfo vertex;
		struct ShaderModuleInfo tessellation_control;
		struct ShaderModuleInfo tessellation_evaluation;
		struct ShaderModuleInfo geometry;
		struct ShaderModuleInfo fragment;
	};

	struct Shader
	{
		ReflectionData reflect_data;
		ft_handle      handle;
	};

	struct DescriptorSetLayout
	{
		ReflectionData reflection_data;
		ft_handle      handle;
	};

	struct VertexBindingInfo
	{
		u32                  binding;
		u32                  stride;
		enum VertexInputRate input_rate;
	};

	struct VertexAttributeInfo
	{
		u32         location;
		u32         binding;
		enum Format format;
		u32         offset;
	};

	struct VertexLayout
	{
		u32                      binding_info_count;
		struct VertexBindingInfo binding_infos[ MAX_VERTEX_BINDING_COUNT ];
		u32                      attribute_info_count;
		struct VertexAttributeInfo
		    attribute_infos[ MAX_VERTEX_ATTRIBUTE_COUNT ];
	};

	struct RasterizerStateInfo
	{
		enum CullMode    cull_mode;
		enum FrontFace   front_face;
		enum PolygonMode polygon_mode;
	};

	struct DepthStateInfo
	{
		b32            depth_test;
		b32            depth_write;
		enum CompareOp compare_op;
	};

	struct BlendStateInfo
	{
		enum BlendFactor src_blend_factors[ MAX_ATTACHMENTS_COUNT ];
		enum BlendFactor dst_blend_factors[ MAX_ATTACHMENTS_COUNT ];
		enum BlendFactor src_alpha_blend_factors[ MAX_ATTACHMENTS_COUNT ];
		enum BlendFactor dst_alpha_blend_factors[ MAX_ATTACHMENTS_COUNT ];
		enum BlendOp     blend_ops[ MAX_ATTACHMENTS_COUNT ];
		enum BlendOp     alpha_blend_ops[ MAX_ATTACHMENTS_COUNT ];
	};

	struct PipelineInfo
	{
		struct VertexLayout         vertex_layout;
		struct RasterizerStateInfo  rasterizer_info;
		enum PrimitiveTopology      topology;
		struct DepthStateInfo       depth_state_info;
		struct Shader*              shader;
		struct DescriptorSetLayout* descriptor_set_layout;
		u32                         sample_count;
		u32                         color_attachment_count;
		enum Format           color_attachment_formats[ MAX_ATTACHMENTS_COUNT ];
		enum Format           depth_stencil_format;
		struct BlendStateInfo blend_state_info;
	};

	struct Pipeline
	{
		enum PipelineType type;
		ft_handle         handle;
	};

	struct DescriptorSetInfo
	{
		u32                         set;
		struct DescriptorSetLayout* descriptor_set_layout;
	};

	struct DescriptorSet
	{
		struct DescriptorSetLayout* layout;
		ft_handle                   handle;
	};

	struct BufferDescriptor
	{
		struct Buffer* buffer;
		u64            offset;
		u64            range;
	};

	struct ImageDescriptor
	{
		struct Image*      image;
		enum ResourceState resource_state;
	};

	struct SamplerDescriptor
	{
		struct Sampler* sampler;
	};

	struct DescriptorWrite
	{
		u32                       descriptor_count;
		const char*               descriptor_name;
		struct SamplerDescriptor* sampler_descriptors;
		struct ImageDescriptor*   image_descriptors;
		struct BufferDescriptor*  buffer_descriptors;
	};

	static inline b32
	format_has_depth_aspect( enum Format format )
	{
		switch ( format )
		{
		case FT_FORMAT_D16_UNORM:
		case FT_FORMAT_D16_UNORMS8_UINT:
		case FT_FORMAT_D24_UNORMS8_UINT:
		case FT_FORMAT_D32_SFLOAT:
		case FT_FORMAT_X8D24_UNORM:
		case FT_FORMAT_D32_SFLOATS8_UINT: return 1;
		default: return 0;
		}
	}

	static inline b32
	format_has_stencil_aspect( enum Format format )
	{
		switch ( format )
		{
		case FT_FORMAT_D16_UNORMS8_UINT:
		case FT_FORMAT_D24_UNORMS8_UINT:
		case FT_FORMAT_D32_SFLOATS8_UINT:
		case FT_FORMAT_S8_UINT: return 1;
		default: return 0;
		}
	}

	void
	create_renderer_backend( const struct RendererBackendInfo* info,
	                         struct RendererBackend**          backend );

	void
	destroy_renderer_backend( struct RendererBackend* backend );

	void
	create_device( const struct RendererBackend* backend,
	               const struct DeviceInfo*      info,
	               struct Device**               device );

	void
	destroy_device( struct Device* device );

	void
	create_queue( const struct Device*    device,
	              const struct QueueInfo* info,
	              struct Queue**          queue );

	void
	destroy_queue( struct Queue* queue );

	void
	queue_wait_idle( const struct Queue* queue );

	void
	queue_submit( const struct Queue*           queue,
	              const struct QueueSubmitInfo* info );

	void
	immediate_submit( const struct Queue* queue, struct CommandBuffer* cmd );

	void
	queue_present( const struct Queue*            queue,
	               const struct QueuePresentInfo* info );

	void
	create_semaphore( const struct Device* device,
	                  struct Semaphore**   semaphore );

	void
	destroy_semaphore( const struct Device* device,
	                   struct Semaphore*    semaphore );

	void
	create_fence( const struct Device* device, struct Fence** fence );

	void
	destroy_fence( const struct Device* device, struct Fence* fence );

	void
	wait_for_fences( const struct Device* device,
	                 u32                  count,
	                 struct Fence**       fences );

	void
	reset_fences( const struct Device* device,
	              u32                  count,
	              struct Fence**       fences );

	void
	create_swapchain( const struct Device*        device,
	                  const struct SwapchainInfo* info,
	                  struct Swapchain**          swapchain );

	void
	resize_swapchain( const struct Device* device,
	                  struct Swapchain*    swapchain,
	                  u32                  width,
	                  u32                  height );

	void
	destroy_swapchain( const struct Device* device,
	                   struct Swapchain*    swapchain );

	void
	create_command_pool( const struct Device*          device,
	                     const struct CommandPoolInfo* info,
	                     struct CommandPool**          command_pool );

	void
	destroy_command_pool( const struct Device* device,
	                      struct CommandPool*  command_pool );

	void
	create_command_buffers( const struct Device*      device,
	                        const struct CommandPool* command_pool,
	                        u32                       count,
	                        struct CommandBuffer**    command_buffers );

	void
	free_command_buffers( const struct Device*      device,
	                      const struct CommandPool* command_pool,
	                      u32                       count,
	                      struct CommandBuffer**    command_buffers );

	void
	destroy_command_buffers( const struct Device*      device,
	                         const struct CommandPool* command_pool,
	                         u32                       count,
	                         struct CommandBuffer**    command_buffers );

	void
	begin_command_buffer( const struct CommandBuffer* cmd );

	void
	end_command_buffer( const struct CommandBuffer* cmd );

	void
	acquire_next_image( const struct Device*    device,
	                    const struct Swapchain* swapchain,
	                    const struct Semaphore* semaphore,
	                    const struct Fence*     fence,
	                    u32*                    image_index );

	void
	create_shader( const struct Device* device,
	               struct ShaderInfo*   info,
	               struct Shader**      shader );

	void
	destroy_shader( const struct Device* device, struct Shader* shader );

	void
	create_descriptor_set_layout(
	    const struct Device*         device,
	    struct Shader*               shader,
	    struct DescriptorSetLayout** descriptor_set_layout );

	void
	destroy_descriptor_set_layout( const struct Device*        device,
	                               struct DescriptorSetLayout* layout );

	void
	create_compute_pipeline( const struct Device*       device,
	                         const struct PipelineInfo* info,
	                         struct Pipeline**          pipeline );

	void
	create_graphics_pipeline( const struct Device*       device,
	                          const struct PipelineInfo* info,
	                          struct Pipeline**          pipeline );

	void
	destroy_pipeline( const struct Device* device, struct Pipeline* pipeline );

	void
	cmd_begin_render_pass( const struct CommandBuffer*       cmd,
	                       const struct RenderPassBeginInfo* info );

	void
	cmd_end_render_pass( const struct CommandBuffer* cmd );

	void
	cmd_barrier( const struct CommandBuffer* cmd,
	             u32                         memory_barriers_count,
	             const struct MemoryBarrier* memory_barrier,
	             u32                         buffer_barriers_count,
	             const struct BufferBarrier* buffer_barriers,
	             u32                         image_barriers_count,
	             const struct ImageBarrier*  image_barriers );

	void
	cmd_set_scissor( const struct CommandBuffer* cmd,
	                 i32                         x,
	                 i32                         y,
	                 u32                         width,
	                 u32                         height );

	void
	cmd_set_viewport( const struct CommandBuffer* cmd,
	                  f32                         x,
	                  f32                         y,
	                  f32                         width,
	                  f32                         height,
	                  f32                         min_depth,
	                  f32                         max_depth );

	void
	cmd_bind_pipeline( const struct CommandBuffer* cmd,
	                   const struct Pipeline*      pipeline );

	void
	cmd_draw( const struct CommandBuffer* cmd,
	          u32                         vertex_count,
	          u32                         instance_count,
	          u32                         first_vertex,
	          u32                         first_instance );

	void
	cmd_draw_indexed( const struct CommandBuffer* cmd,
	                  u32                         index_count,
	                  u32                         instance_count,
	                  u32                         first_index,
	                  i32                         vertex_offset,
	                  u32                         first_instance );

	void
	cmd_bind_vertex_buffer( const struct CommandBuffer* cmd,
	                        const struct Buffer*        buffer,
	                        const u64                   offset );

	void
	cmd_bind_index_buffer_u16( const struct CommandBuffer* cmd,
	                           const struct Buffer*        buffer,
	                           const u64                   offset );

	void
	cmd_bind_index_buffer_u32( const struct CommandBuffer* cmd,
	                           const struct Buffer*        buffer,
	                           const u64                   offset );

	void
	cmd_copy_buffer( const struct CommandBuffer* cmd,
	                 const struct Buffer*        src,
	                 u64                         src_offset,
	                 struct Buffer*              dst,
	                 u64                         dst_offset,
	                 u64                         size );

	void
	cmd_copy_buffer_to_image( const struct CommandBuffer* cmd,
	                          const struct Buffer*        src,
	                          u64                         src_offset,
	                          struct Image*               dst );

	void
	cmd_bind_descriptor_set( const struct CommandBuffer* cmd,
	                         u32                         first_set,
	                         const struct DescriptorSet* set,
	                         const struct Pipeline*      pipeline );

	void
	cmd_dispatch( const struct CommandBuffer* cmd,
	              u32                         group_count_x,
	              u32                         group_count_y,
	              u32                         group_count_z );

	void
	cmd_push_constants( const struct CommandBuffer* cmd,
	                    const struct Pipeline*      pipeline,
	                    u32                         offset,
	                    u32                         size,
	                    const void*                 data );

	void
	create_buffer( const struct Device*     device,
	               const struct BufferInfo* info,
	               struct Buffer**          buffer );

	void
	destroy_buffer( const struct Device* device, struct Buffer* buffer );

	void*
	map_memory( const struct Device* device, struct Buffer* buffer );

	void
	unmap_memory( const struct Device* device, struct Buffer* buffer );

	void
	cmd_draw_indexed_indirect( const struct CommandBuffer* cmd,
	                           const struct Buffer*        buffer,
	                           u64                         offset,
	                           u32                         draw_count,
	                           u32                         stride );

	void
	create_sampler( const struct Device*      device,
	                const struct SamplerInfo* info,
	                struct Sampler**          sampler );

	void
	destroy_sampler( const struct Device* device, struct Sampler* sampler );

	void
	create_image( const struct Device*    device,
	              const struct ImageInfo* info,
	              struct Image**          image );

	void
	destroy_image( const struct Device* device, struct Image* image );

	void
	create_descriptor_set( const struct Device*            device,
	                       const struct DescriptorSetInfo* info,
	                       struct DescriptorSet**          descriptor_set );

	void
	destroy_descriptor_set( const struct Device*  device,
	                        struct DescriptorSet* set );

	void
	update_descriptor_set( const struct Device*          device,
	                       struct DescriptorSet*         set,
	                       u32                           count,
	                       const struct DescriptorWrite* writes );

	void
	begin_upload_batch( void );

	void
	end_upload_batch( void );

	void
	upload_buffer( struct Buffer* buffer,
	               u64            offset,
	               u64            size,
	               const void*    data );

	void
	upload_image( struct Image* image, u64 size, const void* data );

#ifdef __cplusplus
}
#endif
