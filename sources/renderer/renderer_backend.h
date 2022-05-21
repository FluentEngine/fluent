#pragma once

#include "base/base.h"

#include "renderer/renderer_enums.h"
#include "renderer/shader_reflection.h"

#undef MemoryBarrier

#ifdef __cplusplus
extern "C"
{
#endif

	struct WsiInfo;

#define FT_INIT_INTERNAL( name, ptr, type )                                    \
	type* name             = ( type* ) calloc( 1, sizeof( type ) );            \
	name->interface.handle = name;                                             \
	ptr                    = &name->interface

#define FT_FROM_HANDLE( name, interface, impl )                                \
	impl* name = ( impl* ) interface->handle

#define DECLARE_FUNCTION_POINTER( ret, name, ... )                             \
	typedef ret ( *name##_fun )( __VA_ARGS__ );                                \
	extern name##_fun name

	// Forward declares
	struct Window;
	struct Buffer;
	struct StagingBuffer;
	struct Image;
	struct Queue;
	struct CommandBuffer;
	struct CommandPool;

#define MAX_DEVICE_COUNT             1
#define MAX_ATTACHMENTS_COUNT        10
#define MAX_PUSH_CONSTANT_RANGE      128
#define MAX_VERTEX_BINDING_COUNT     15
#define MAX_VERTEX_ATTRIBUTE_COUNT   15
#define MAX_DESCRIPTOR_BINDING_COUNT 15
#define MAX_SET_COUNT                10

	typedef void* Handle;

	typedef struct RendererBackendInfo
	{
		RendererAPI     api;
		struct WsiInfo* wsi_info;
	} RendererBackendInfo;

	typedef struct RendererBackend
	{
		Handle handle;
	} RendererBackend;

	typedef struct DeviceInfo
	{
		u32 frame_in_use_count;
	} DeviceInfo;

	typedef struct Device
	{
		Handle handle;
	} Device;

	typedef struct CommandPoolInfo
	{
		struct Queue* queue;
	} CommandPoolInfo;

	typedef struct CommandPool
	{
		struct Queue* queue;
		Handle        handle;
	} CommandPool;

	typedef struct CommandBuffer
	{
		struct Queue* queue;
		Handle        handle;
	} CommandBuffer;

	typedef struct QueueInfo
	{
		QueueType queue_type;
	} QueueInfo;

	typedef struct Queue
	{
		u32       family_index;
		QueueType type;
		Handle    handle;
	} Queue;

	typedef struct Semaphore
	{
		Handle handle;
	} Semaphore;

	typedef struct Fence
	{
		Handle handle;
	} Fence;

	typedef struct SamplerInfo
	{
		Filter             mag_filter;
		Filter             min_filter;
		SamplerMipmapMode  mipmap_mode;
		SamplerAddressMode address_mode_u;
		SamplerAddressMode address_mode_v;
		SamplerAddressMode address_mode_w;
		f32                mip_lod_bias;
		b32                anisotropy_enable;
		f32                max_anisotropy;
		b32                compare_enable;
		CompareOp          compare_op;
		f32                min_lod;
		f32                max_lod;
	} SamplerInfo;

	typedef struct Sampler
	{
		Handle handle;
	} Sampler;

	typedef struct ImageInfo
	{
		u32            width;
		u32            height;
		u32            depth;
		Format         format;
		u32            sample_count;
		u32            layer_count;
		u32            mip_levels;
		DescriptorType descriptor_type;
	} ImageInfo;

	typedef struct Image
	{
		u32            width;
		u32            height;
		u32            depth;
		Format         format;
		u32            sample_count;
		u32            mip_level_count;
		u32            layer_count;
		DescriptorType descriptor_type;
		Handle         handle;
	} Image;

	typedef struct BufferInfo
	{
		u64            size;
		DescriptorType descriptor_type;
		MemoryUsage    memory_usage;
	} BufferInfo;

	typedef struct Buffer
	{
		u64            size;
		ResourceState  resource_state;
		DescriptorType descriptor_type;
		MemoryUsage    memory_usage;
		void*          mapped_memory;
		Handle         handle;
	} Buffer;

	typedef struct SwapchainInfo
	{
		Queue*          queue;
		u32             width;
		u32             height;
		Format          format;
		b32             vsync;
		u32             min_image_count;
		struct WsiInfo* wsi_info;
	} SwapchainInfo;

	typedef struct Swapchain
	{
		u32     min_image_count;
		u32     image_count;
		u32     width;
		u32     height;
		Format  format;
		Image** images;
		Queue*  queue;
		b32     vsync;
		Handle  handle;
	} Swapchain;

	typedef struct QueueSubmitInfo
	{
		u32             wait_semaphore_count;
		Semaphore**     wait_semaphores;
		u32             command_buffer_count;
		CommandBuffer** command_buffers;
		u32             signal_semaphore_count;
		Semaphore**     signal_semaphores;
		Fence*          signal_fence;
	} QueueSubmitInfo;

	typedef struct QueuePresentInfo
	{
		u32         wait_semaphore_count;
		Semaphore** wait_semaphores;
		Swapchain*  swapchain;
		u32         image_index;
	} QueuePresentInfo;

	typedef f32 ColorClearValue[ 4 ];

	typedef struct DepthStencilClearValue
	{
		f32 depth;
		u32 stencil;
	} DepthStencilClearValue;

	typedef struct ClearValue
	{
		ColorClearValue        color;
		DepthStencilClearValue depth_stencil;
	} ClearValue;

	typedef struct RenderPassBeginInfo
	{
		const Device*    device;
		u32              width;
		u32              height;
		u32              color_attachment_count;
		Image*           color_attachments[ MAX_ATTACHMENTS_COUNT ];
		AttachmentLoadOp color_attachment_load_ops[ MAX_ATTACHMENTS_COUNT ];
		ResourceState    color_image_states[ MAX_ATTACHMENTS_COUNT ];
		Image*           depth_stencil;
		AttachmentLoadOp depth_stencil_load_op;
		ResourceState    depth_stencil_state;
		ClearValue       clear_values[ MAX_ATTACHMENTS_COUNT + 1 ];
	} RenderPassBeginInfo;

	// TODO:
	typedef struct MemoryBarrier
	{
	} MemoryBarrier;

	typedef struct BufferBarrier
	{
		ResourceState old_state;
		ResourceState new_state;
		Buffer*       buffer;
		Queue*        src_queue;
		Queue*        dst_queue;
		u32           size;
		u32           offset;
	} BufferBarrier;

	typedef struct ImageBarrier
	{
		ResourceState old_state;
		ResourceState new_state;
		const Image*  image;
		const Queue*  src_queue;
		const Queue*  dst_queue;
	} ImageBarrier;

	typedef struct ShaderModuleInfo
	{
		u32         bytecode_size;
		const void* bytecode;
	} ShaderModuleInfo;

	typedef struct ShaderInfo
	{
		ShaderModuleInfo compute;
		ShaderModuleInfo vertex;
		ShaderModuleInfo tessellation_control;
		ShaderModuleInfo tessellation_evaluation;
		ShaderModuleInfo geometry;
		ShaderModuleInfo fragment;
	} ShaderInfo;

	typedef struct Shader
	{
		ReflectionData reflect_data;
		Handle         handle;
	} Shader;

	typedef struct DescriptorSetLayout
	{
		ReflectionData reflection_data;
		Handle         handle;
	} DescriptorSetLayout;

	typedef struct VertexBindingInfo
	{
		u32             binding;
		u32             stride;
		VertexInputRate input_rate;
	} VertexBindingInfo;

	typedef struct VertexAttributeInfo
	{
		u32    location;
		u32    binding;
		Format format;
		u32    offset;
	} VertexAttributeInfo;

	typedef struct VertexLayout
	{
		u32                 binding_info_count;
		VertexBindingInfo   binding_infos[ MAX_VERTEX_BINDING_COUNT ];
		u32                 attribute_info_count;
		VertexAttributeInfo attribute_infos[ MAX_VERTEX_ATTRIBUTE_COUNT ];
	} VertexLayout;

	typedef struct RasterizerStateInfo
	{
		CullMode    cull_mode;
		FrontFace   front_face;
		PolygonMode polygon_mode;
	} RasterizerStateInfo;

	typedef struct DepthStateInfo
	{
		b32       depth_test;
		b32       depth_write;
		CompareOp compare_op;
	} DepthStateInfo;

	typedef struct PipelineInfo
	{
		VertexLayout         vertex_layout;
		RasterizerStateInfo  rasterizer_info;
		PrimitiveTopology    topology;
		DepthStateInfo       depth_state_info;
		Shader*              shader;
		DescriptorSetLayout* descriptor_set_layout;
		u32                  sample_count;
		u32                  color_attachment_count;
		Format               color_attachment_formats[ MAX_ATTACHMENTS_COUNT ];
		Format               depth_stencil_format;
	} PipelineInfo;

	typedef struct Pipeline
	{
		PipelineType type;
		Handle       handle;
	} Pipeline;

	typedef struct DescriptorSetInfo
	{
		u32                  set;
		DescriptorSetLayout* descriptor_set_layout;
	} DescriptorSetInfo;

	typedef struct DescriptorSet
	{
		DescriptorSetLayout* layout;
		Handle               handle;
	} DescriptorSet;

	typedef struct BufferDescriptor
	{
		Buffer* buffer;
		u64     offset;
		u64     range;
	} BufferDescriptor;

	typedef struct ImageDescriptor
	{
		Image*        image;
		ResourceState resource_state;
	} ImageDescriptor;

	typedef struct SamplerDescriptor
	{
		Sampler* sampler;
	} SamplerDescriptor;

	typedef struct DescriptorWrite
	{
		u32                descriptor_count;
		const char*        descriptor_name;
		SamplerDescriptor* sampler_descriptors;
		ImageDescriptor*   image_descriptors;
		BufferDescriptor*  buffer_descriptors;
	} DescriptorWrite;

	static inline b32
	format_has_depth_aspect( Format format )
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
	format_has_stencil_aspect( Format format )
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
	create_renderer_backend( const RendererBackendInfo* info,
	                         RendererBackend**          backend );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_renderer_backend,
	                          RendererBackend* backend );

	DECLARE_FUNCTION_POINTER( void,
	                          create_device,
	                          const RendererBackend* backend,
	                          const DeviceInfo*      info,
	                          Device**               device );

	DECLARE_FUNCTION_POINTER( void, destroy_device, Device* device );

	DECLARE_FUNCTION_POINTER( void,
	                          create_queue,
	                          const Device*    device,
	                          const QueueInfo* info,
	                          Queue**          queue );

	DECLARE_FUNCTION_POINTER( void, destroy_queue, Queue* queue );

	DECLARE_FUNCTION_POINTER( void, queue_wait_idle, const Queue* queue );

	DECLARE_FUNCTION_POINTER( void,
	                          queue_submit,
	                          const Queue*           queue,
	                          const QueueSubmitInfo* info );

	DECLARE_FUNCTION_POINTER( void,
	                          immediate_submit,
	                          const Queue*   queue,
	                          CommandBuffer* cmd );

	DECLARE_FUNCTION_POINTER( void,
	                          queue_present,
	                          const Queue*            queue,
	                          const QueuePresentInfo* info );

	DECLARE_FUNCTION_POINTER( void,
	                          create_semaphore,
	                          const Device* device,
	                          Semaphore**   semaphore );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_semaphore,
	                          const Device* device,
	                          Semaphore*    semaphore );

	DECLARE_FUNCTION_POINTER( void,
	                          create_fence,
	                          const Device* device,
	                          Fence**       fence );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_fence,
	                          const Device* device,
	                          Fence*        fence );

	DECLARE_FUNCTION_POINTER( void,
	                          wait_for_fences,
	                          const Device* device,
	                          u32           count,
	                          Fence**       fences );

	DECLARE_FUNCTION_POINTER( void,
	                          reset_fences,
	                          const Device* device,
	                          u32           count,
	                          Fence**       fences );

	DECLARE_FUNCTION_POINTER( void,
	                          create_swapchain,
	                          const Device*        device,
	                          const SwapchainInfo* info,
	                          Swapchain**          swapchain );

	DECLARE_FUNCTION_POINTER( void,
	                          resize_swapchain,
	                          const Device* device,
	                          Swapchain*    swapchain,
	                          u32           width,
	                          u32           height );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_swapchain,
	                          const Device* device,
	                          Swapchain*    swapchain );

	DECLARE_FUNCTION_POINTER( void,
	                          create_command_pool,
	                          const Device*          device,
	                          const CommandPoolInfo* info,
	                          CommandPool**          command_pool );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_command_pool,
	                          const Device* device,
	                          CommandPool*  command_pool );

	DECLARE_FUNCTION_POINTER( void,
	                          create_command_buffers,
	                          const Device*      device,
	                          const CommandPool* command_pool,
	                          u32                count,
	                          CommandBuffer**    command_buffers );

	DECLARE_FUNCTION_POINTER( void,
	                          free_command_buffers,
	                          const Device*      device,
	                          const CommandPool* command_pool,
	                          u32                count,
	                          CommandBuffer**    command_buffers );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_command_buffers,
	                          const Device*      device,
	                          const CommandPool* command_pool,
	                          u32                count,
	                          CommandBuffer**    command_buffers );

	DECLARE_FUNCTION_POINTER( void,
	                          begin_command_buffer,
	                          const CommandBuffer* cmd );

	DECLARE_FUNCTION_POINTER( void,
	                          end_command_buffer,
	                          const CommandBuffer* cmd );

	DECLARE_FUNCTION_POINTER( void,
	                          acquire_next_image,
	                          const Device*    device,
	                          const Swapchain* swapchain,
	                          const Semaphore* semaphore,
	                          const Fence*     fence,
	                          u32*             image_index );

	DECLARE_FUNCTION_POINTER( void,
	                          create_shader,
	                          const Device* device,
	                          ShaderInfo*   info,
	                          Shader**      shader );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_shader,
	                          const Device* device,
	                          Shader*       shader );

	DECLARE_FUNCTION_POINTER( void,
	                          create_descriptor_set_layout,
	                          const Device*         device,
	                          Shader*               shader,
	                          DescriptorSetLayout** descriptor_set_layout );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_descriptor_set_layout,
	                          const Device*        device,
	                          DescriptorSetLayout* layout );

	DECLARE_FUNCTION_POINTER( void,
	                          create_compute_pipeline,
	                          const Device*       device,
	                          const PipelineInfo* info,
	                          Pipeline**          pipeline );

	DECLARE_FUNCTION_POINTER( void,
	                          create_graphics_pipeline,
	                          const Device*       device,
	                          const PipelineInfo* info,
	                          Pipeline**          pipeline );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_pipeline,
	                          const Device* device,
	                          Pipeline*     pipeline );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_begin_render_pass,
	                          const CommandBuffer*       cmd,
	                          const RenderPassBeginInfo* info );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_end_render_pass,
	                          const CommandBuffer* cmd );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_barrier,
	                          const CommandBuffer* cmd,
	                          u32                  memory_barriers_count,
	                          const MemoryBarrier* memory_barrier,
	                          u32                  buffer_barriers_count,
	                          const BufferBarrier* buffer_barriers,
	                          u32                  image_barriers_count,
	                          const ImageBarrier*  image_barriers );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_set_scissor,
	                          const CommandBuffer* cmd,
	                          i32                  x,
	                          i32                  y,
	                          u32                  width,
	                          u32                  height );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_set_viewport,
	                          const CommandBuffer* cmd,
	                          f32                  x,
	                          f32                  y,
	                          f32                  width,
	                          f32                  height,
	                          f32                  min_depth,
	                          f32                  max_depth );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_bind_pipeline,
	                          const CommandBuffer* cmd,
	                          const Pipeline*      pipeline );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_draw,
	                          const CommandBuffer* cmd,
	                          u32                  vertex_count,
	                          u32                  instance_count,
	                          u32                  first_vertex,
	                          u32                  first_instance );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_draw_indexed,
	                          const CommandBuffer* cmd,
	                          u32                  index_count,
	                          u32                  instance_count,
	                          u32                  first_index,
	                          i32                  vertex_offset,
	                          u32                  first_instance );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_bind_vertex_buffer,
	                          const CommandBuffer* cmd,
	                          const Buffer*        buffer,
	                          const u64            offset );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_bind_index_buffer_u16,
	                          const CommandBuffer* cmd,
	                          const Buffer*        buffer,
	                          const u64            offset );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_bind_index_buffer_u32,
	                          const CommandBuffer* cmd,
	                          const Buffer*        buffer,
	                          const u64            offset );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_copy_buffer,
	                          const CommandBuffer* cmd,
	                          const Buffer*        src,
	                          u64                  src_offset,
	                          Buffer*              dst,
	                          u64                  dst_offset,
	                          u64                  size );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_copy_buffer_to_image,
	                          const CommandBuffer* cmd,
	                          const Buffer*        src,
	                          u64                  src_offset,
	                          Image*               dst );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_bind_descriptor_set,
	                          const CommandBuffer* cmd,
	                          u32                  first_set,
	                          const DescriptorSet* set,
	                          const Pipeline*      pipeline );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_dispatch,
	                          const CommandBuffer* cmd,
	                          u32                  group_count_x,
	                          u32                  group_count_y,
	                          u32                  group_count_z );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_push_constants,
	                          const CommandBuffer* cmd,
	                          const Pipeline*      pipeline,
	                          u32                  offset,
	                          u32                  size,
	                          const void*          data );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_blit_image,
	                          const CommandBuffer* cmd,
	                          const Image*         src,
	                          ResourceState        src_state,
	                          Image*               dst,
	                          ResourceState        dst_state,
	                          Filter               filter );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_clear_color_image,
	                          const CommandBuffer* cmd,
	                          Image*               image,
	                          float                color[ 4 ] );

	DECLARE_FUNCTION_POINTER( void,
	                          create_buffer,
	                          const Device*     device,
	                          const BufferInfo* info,
	                          Buffer**          buffer );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_buffer,
	                          const Device* device,
	                          Buffer*       buffer );

	DECLARE_FUNCTION_POINTER( void*,
	                          map_memory,
	                          const Device* device,
	                          Buffer*       buffer );

	DECLARE_FUNCTION_POINTER( void,
	                          unmap_memory,
	                          const Device* device,
	                          Buffer*       buffer );

	DECLARE_FUNCTION_POINTER( void,
	                          cmd_draw_indexed_indirect,
	                          const CommandBuffer* cmd,
	                          const Buffer*        buffer,
	                          u64                  offset,
	                          u32                  draw_count,
	                          u32                  stride );

	DECLARE_FUNCTION_POINTER( void,
	                          create_sampler,
	                          const Device*      device,
	                          const SamplerInfo* info,
	                          Sampler**          sampler );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_sampler,
	                          const Device* device,
	                          Sampler*      sampler );

	DECLARE_FUNCTION_POINTER( void,
	                          create_image,
	                          const Device*    device,
	                          const ImageInfo* info,
	                          Image**          image );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_image,
	                          const Device* device,
	                          Image*        image );

	DECLARE_FUNCTION_POINTER( void,
	                          create_descriptor_set,
	                          const Device*            device,
	                          const DescriptorSetInfo* info,
	                          DescriptorSet**          descriptor_set );

	DECLARE_FUNCTION_POINTER( void,
	                          destroy_descriptor_set,
	                          const Device*  device,
	                          DescriptorSet* set );

	DECLARE_FUNCTION_POINTER( void,
	                          update_descriptor_set,
	                          const Device*          device,
	                          DescriptorSet*         set,
	                          u32                    count,
	                          const DescriptorWrite* writes );
#ifdef __cplusplus
}
#endif
