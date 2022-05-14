#pragma once

#include "core/base.hpp"
#include "math/math.hpp"
#include "renderer/renderer_enums.hpp"
#include "renderer/shader_reflection.hpp"

#undef MemoryBarrier

namespace fluent
{
#define FT_INIT_INTERNAL( name, ptr, type )                                    \
	type* name = static_cast<type*>( std::calloc( 1, sizeof( type ) ) );       \
	name->interface.handle = name;                                             \
	ptr                    = &name->interface;

#define FT_FROM_HANDLE( name, interface, impl )                                \
	impl* name = static_cast<impl*>( interface->handle );

#define DECLARE_RENDERER_FUNCTION( ret, name, ... )                            \
	typedef ret ( *name##_fun )( __VA_ARGS__ );                                \
	extern name##_fun name;

// Forward declares
struct Window;
struct Buffer;
struct StagingBuffer;
struct Image;
struct Queue;
struct CommandBuffer;
struct CommandPool;

static constexpr u32 MAX_DEVICE_COUNT             = 1;
static constexpr u32 MAX_ATTACHMENTS_COUNT        = 10;
static constexpr u32 MAX_PUSH_CONSTANT_RANGE      = 128;
static constexpr u32 MAX_VERTEX_BINDING_COUNT     = 15;
static constexpr u32 MAX_VERTEX_ATTRIBUTE_COUNT   = 15;
static constexpr u32 MAX_DESCRIPTOR_BINDING_COUNT = 15;
static constexpr u32 MAX_SET_COUNT                = 10;

using Handle = void*;

struct RendererBackendInfo
{
	RendererAPI api;
};

struct RendererBackend
{
	Handle handle;
};

struct DeviceInfo
{
	u32 frame_in_use_count;
};

struct Device
{
	Handle handle;
};

struct CommandPoolInfo
{
	Queue* queue;
};

struct CommandPool
{
	Queue* queue;
	Handle handle;
};

struct CommandBuffer
{
	Queue* queue;
	Handle handle;
};

struct QueueInfo
{
	QueueType queue_type;
};

struct Queue
{
	u32       family_index;
	QueueType type;
	Handle    handle;
};

struct Semaphore
{
	Handle handle;
};

struct Fence
{
	Handle handle;
};

struct SamplerInfo
{
	Filter             mag_filter;
	Filter             min_filter;
	SamplerMipmapMode  mipmap_mode;
	SamplerAddressMode address_mode_u;
	SamplerAddressMode address_mode_v;
	SamplerAddressMode address_mode_w;
	f32                mip_lod_bias;
	bool               anisotropy_enable;
	f32                max_anisotropy;
	b32                compare_enable;
	CompareOp          compare_op;
	f32                min_lod;
	f32                max_lod;
};

struct Sampler
{
	Handle handle;
};

struct ImageInfo
{
	u32            width;
	u32            height;
	u32            depth;
	Format         format;
	SampleCount    sample_count;
	u32            layer_count = 1;
	u32            mip_levels  = 1;
	DescriptorType descriptor_type;
};

struct Image
{
	u32            width;
	u32            height;
	u32            depth;
	Format         format;
	SampleCount    sample_count;
	u32            mip_level_count = 1;
	u32            layer_count     = 1;
	DescriptorType descriptor_type;
	Handle         handle;
};

struct BufferInfo
{
	u64            size = 0;
	DescriptorType descriptor_type;
	MemoryUsage    memory_usage;
};

struct Buffer
{
	u64            size;
	ResourceState  resource_state;
	DescriptorType descriptor_type;
	MemoryUsage    memory_usage;
	void*          mapped_memory;
	Handle         handle;
};

struct SwapchainInfo
{
	Queue* queue;
	u32    width;
	u32    height;
	Format format;
	b32    vsync;
	u32    min_image_count;
};

struct Swapchain
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
};

struct QueueSubmitInfo
{
	u32             wait_semaphore_count;
	Semaphore**     wait_semaphores;
	u32             command_buffer_count;
	CommandBuffer** command_buffers;
	u32             signal_semaphore_count;
	Semaphore**     signal_semaphores;
	Fence*          signal_fence;
};

struct QueuePresentInfo
{
	u32         wait_semaphore_count;
	Semaphore** wait_semaphores;
	Swapchain*  swapchain;
	u32         image_index;
};

using ColorClearValue = f32[ 4 ];

struct DepthStencilClearValue
{
	f32 depth;
	u32 stencil;
};

struct ClearValue
{
	ColorClearValue        color;
	DepthStencilClearValue depth_stencil;
};

struct RenderPassBeginInfo
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
};

// TODO:
struct MemoryBarrier
{
};

struct BufferBarrier
{
	ResourceState old_state;
	ResourceState new_state;
	Buffer*       buffer;
	Queue*        src_queue;
	Queue*        dst_queue;
	u32           size;
	u32           offset;
};

struct ImageBarrier
{
	ResourceState old_state;
	ResourceState new_state;
	const Image*  image;
	const Queue*  src_queue;
	const Queue*  dst_queue;
};

struct ShaderModuleInfo
{
	u32         bytecode_size;
	const void* bytecode;
};

struct ShaderInfo
{
	ShaderModuleInfo compute;
	ShaderModuleInfo vertex;
	ShaderModuleInfo tessellation_control;
	ShaderModuleInfo tessellation_evaluation;
	ShaderModuleInfo geometry;
	ShaderModuleInfo fragment;
};

struct Shader
{
	ReflectionData reflect_data;
	Handle         handle;
};

struct DescriptorSetLayout
{
	ReflectionData reflection_data;
	Handle         handle;
};

struct VertexBindingInfo
{
	u32             binding;
	u32             stride;
	VertexInputRate input_rate;
};

struct VertexAttributeInfo
{
	u32    location;
	u32    binding;
	Format format;
	u32    offset;
};

struct VertexLayout
{
	u32                 binding_info_count;
	VertexBindingInfo   binding_infos[ MAX_VERTEX_BINDING_COUNT ];
	u32                 attribute_info_count;
	VertexAttributeInfo attribute_infos[ MAX_VERTEX_ATTRIBUTE_COUNT ];
};

struct RasterizerStateInfo
{
	CullMode    cull_mode;
	FrontFace   front_face;
	PolygonMode polygon_mode;
};

struct DepthStateInfo
{
	b32       depth_test;
	b32       depth_write;
	CompareOp compare_op;
};

struct PipelineInfo
{
	VertexLayout         vertex_layout;
	RasterizerStateInfo  rasterizer_info;
	PrimitiveTopology    topology;
	DepthStateInfo       depth_state_info;
	Shader*              shader;
	DescriptorSetLayout* descriptor_set_layout;
	SampleCount          sample_count;
	u32                  color_attachment_count;
	Format               color_attachment_formats[ MAX_ATTACHMENTS_COUNT ];
	Format               depth_stencil_format;
};

struct Pipeline
{
	PipelineType type;
	Handle       handle;
};

struct DescriptorSetInfo
{
	u32                  set = 0;
	DescriptorSetLayout* descriptor_set_layout;
};

struct DescriptorSet
{
	DescriptorSetLayout* layout;
	Handle               handle;
};

struct BufferDescriptor
{
	Buffer* buffer;
	u64     offset = 0;
	u64     range  = 0;
};

struct ImageDescriptor
{
	Image*        image;
	ResourceState resource_state;
};

struct SamplerDescriptor
{
	Sampler* sampler;
};

struct DescriptorWrite
{
	u32                descriptor_count;
	const char*        descriptor_name;
	SamplerDescriptor* sampler_descriptors;
	ImageDescriptor*   image_descriptors;
	BufferDescriptor*  buffer_descriptors;
};

struct UiInfo
{
	const Window*          window;
	const RendererBackend* backend;
	const Device*          device;
	const Queue*           queue;
	u32                    color_attachment_count;
	SampleCount            sample_count;
	Format                 color_attachment_formats[ MAX_ATTACHMENTS_COUNT ];
	Format                 depth_stencil_format;
	u32                    min_image_count;
	u32                    image_count;
	u32                    in_fly_frame_count;
	b32                    docking   = false;
	b32                    viewports = false;
};

inline b32
format_has_depth_aspect( Format format )
{
	switch ( format )
	{
	case Format::D16_UNORM:
	case Format::D16_UNORMS8_UINT:
	case Format::D24_UNORMS8_UINT:
	case Format::D32_SFLOAT:
	case Format::X8D24_UNORM:
	case Format::D32_SFLOATS8_UINT: return true;
	default: return false;
	}
}

inline b32
format_has_stencil_aspect( Format format )
{
	switch ( format )
	{
	case Format::D16_UNORMS8_UINT:
	case Format::D24_UNORMS8_UINT:
	case Format::D32_SFLOATS8_UINT:
	case Format::S8_UINT: return true;
	default: return false;
	}
}

template <class T>
inline void
hash_combine( std::size_t& s, const T& v )
{
	std::hash<T> h;
	s ^= h( v ) + 0x9e3779b9 + ( s << 6 ) + ( s >> 2 );
}

void
create_renderer_backend( const RendererBackendInfo* info,
                         RendererBackend**          backend );

DECLARE_RENDERER_FUNCTION( void,
                           destroy_renderer_backend,
                           RendererBackend* backend );

DECLARE_RENDERER_FUNCTION( void,
                           create_device,
                           const RendererBackend* backend,
                           const DeviceInfo*      info,
                           Device**               device );

DECLARE_RENDERER_FUNCTION( void, destroy_device, Device* device );

DECLARE_RENDERER_FUNCTION( void,
                           create_queue,
                           const Device*    device,
                           const QueueInfo* info,
                           Queue**          queue );

DECLARE_RENDERER_FUNCTION( void, destroy_queue, Queue* queue );

DECLARE_RENDERER_FUNCTION( void, queue_wait_idle, const Queue* queue );

DECLARE_RENDERER_FUNCTION( void,
                           queue_submit,
                           const Queue*           queue,
                           const QueueSubmitInfo* info );

DECLARE_RENDERER_FUNCTION( void,
                           immediate_submit,
                           const Queue*   queue,
                           CommandBuffer* cmd );

DECLARE_RENDERER_FUNCTION( void,
                           queue_present,
                           const Queue*            queue,
                           const QueuePresentInfo* info );

DECLARE_RENDERER_FUNCTION( void,
                           create_semaphore,
                           const Device* device,
                           Semaphore**   semaphore );

DECLARE_RENDERER_FUNCTION( void,
                           destroy_semaphore,
                           const Device* device,
                           Semaphore*    semaphore );

DECLARE_RENDERER_FUNCTION( void,
                           create_fence,
                           const Device* device,
                           Fence**       fence );

DECLARE_RENDERER_FUNCTION( void,
                           destroy_fence,
                           const Device* device,
                           Fence*        fence );

DECLARE_RENDERER_FUNCTION( void,
                           wait_for_fences,
                           const Device* device,
                           u32           count,
                           Fence**       fences );

DECLARE_RENDERER_FUNCTION( void,
                           reset_fences,
                           const Device* device,
                           u32           count,
                           Fence**       fences );

DECLARE_RENDERER_FUNCTION( void,
                           create_swapchain,
                           const Device*        device,
                           const SwapchainInfo* info,
                           Swapchain**          swapchain );

DECLARE_RENDERER_FUNCTION( void,
                           resize_swapchain,
                           const Device* device,
                           Swapchain*    swapchain,
                           u32           width,
                           u32           height );

DECLARE_RENDERER_FUNCTION( void,
                           destroy_swapchain,
                           const Device* device,
                           Swapchain*    swapchain );

DECLARE_RENDERER_FUNCTION( void,
                           create_command_pool,
                           const Device*          device,
                           const CommandPoolInfo* info,
                           CommandPool**          command_pool );

DECLARE_RENDERER_FUNCTION( void,
                           destroy_command_pool,
                           const Device* device,
                           CommandPool*  command_pool );

DECLARE_RENDERER_FUNCTION( void,
                           create_command_buffers,
                           const Device*      device,
                           const CommandPool* command_pool,
                           u32                count,
                           CommandBuffer**    command_buffers );

DECLARE_RENDERER_FUNCTION( void,
                           free_command_buffers,
                           const Device*      device,
                           const CommandPool* command_pool,
                           u32                count,
                           CommandBuffer**    command_buffers );

DECLARE_RENDERER_FUNCTION( void,
                           destroy_command_buffers,
                           const Device*      device,
                           const CommandPool* command_pool,
                           u32                count,
                           CommandBuffer**    command_buffers );

DECLARE_RENDERER_FUNCTION( void,
                           begin_command_buffer,
                           const CommandBuffer* cmd );

DECLARE_RENDERER_FUNCTION( void, end_command_buffer, const CommandBuffer* cmd );

DECLARE_RENDERER_FUNCTION( void,
                           acquire_next_image,
                           const Device*    device,
                           const Swapchain* swapchain,
                           const Semaphore* semaphore,
                           const Fence*     fence,
                           u32*             image_index );

DECLARE_RENDERER_FUNCTION( void,
                           create_shader,
                           const Device* device,
                           ShaderInfo*   info,
                           Shader**      shader );

DECLARE_RENDERER_FUNCTION( void,
                           destroy_shader,
                           const Device* device,
                           Shader*       shader );

DECLARE_RENDERER_FUNCTION( void,
                           create_descriptor_set_layout,
                           const Device*         device,
                           Shader*               shader,
                           DescriptorSetLayout** descriptor_set_layout );

DECLARE_RENDERER_FUNCTION( void,
                           destroy_descriptor_set_layout,
                           const Device*        device,
                           DescriptorSetLayout* layout );

DECLARE_RENDERER_FUNCTION( void,
                           create_compute_pipeline,
                           const Device*       device,
                           const PipelineInfo* info,
                           Pipeline**          pipeline );

DECLARE_RENDERER_FUNCTION( void,
                           create_graphics_pipeline,
                           const Device*       device,
                           const PipelineInfo* info,
                           Pipeline**          pipeline );

DECLARE_RENDERER_FUNCTION( void,
                           destroy_pipeline,
                           const Device* device,
                           Pipeline*     pipeline );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_begin_render_pass,
                           const CommandBuffer*       cmd,
                           const RenderPassBeginInfo* info );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_end_render_pass,
                           const CommandBuffer* cmd );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_barrier,
                           const CommandBuffer* cmd,
                           u32                  memory_barriers_count,
                           const MemoryBarrier* memory_barrier,
                           u32                  buffer_barriers_count,
                           const BufferBarrier* buffer_barriers,
                           u32                  image_barriers_count,
                           const ImageBarrier*  image_barriers );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_set_scissor,
                           const CommandBuffer* cmd,
                           i32                  x,
                           i32                  y,
                           u32                  width,
                           u32                  height );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_set_viewport,
                           const CommandBuffer* cmd,
                           f32                  x,
                           f32                  y,
                           f32                  width,
                           f32                  height,
                           f32                  min_depth,
                           f32                  max_depth );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_bind_pipeline,
                           const CommandBuffer* cmd,
                           const Pipeline*      pipeline );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_draw,
                           const CommandBuffer* cmd,
                           u32                  vertex_count,
                           u32                  instance_count,
                           u32                  first_vertex,
                           u32                  first_instance );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_draw_indexed,
                           const CommandBuffer* cmd,
                           u32                  index_count,
                           u32                  instance_count,
                           u32                  first_index,
                           i32                  vertex_offset,
                           u32                  first_instance );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_bind_vertex_buffer,
                           const CommandBuffer* cmd,
                           const Buffer*        buffer,
                           const u64            offset );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_bind_index_buffer_u16,
                           const CommandBuffer* cmd,
                           const Buffer*        buffer,
                           const u64            offset );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_bind_index_buffer_u32,
                           const CommandBuffer* cmd,
                           const Buffer*        buffer,
                           const u64            offset );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_copy_buffer,
                           const CommandBuffer* cmd,
                           const Buffer*        src,
                           u64                  src_offset,
                           Buffer*              dst,
                           u64                  dst_offset,
                           u64                  size );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_copy_buffer_to_image,
                           const CommandBuffer* cmd,
                           const Buffer*        src,
                           u64                  src_offset,
                           Image*               dst );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_bind_descriptor_set,
                           const CommandBuffer* cmd,
                           u32                  first_set,
                           const DescriptorSet* set,
                           const Pipeline*      pipeline );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_dispatch,
                           const CommandBuffer* cmd,
                           u32                  group_count_x,
                           u32                  group_count_y,
                           u32                  group_count_z );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_push_constants,
                           const CommandBuffer* cmd,
                           const Pipeline*      pipeline,
                           u64                  offset,
                           u64                  size,
                           const void*          data );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_blit_image,
                           const CommandBuffer* cmd,
                           const Image*         src,
                           ResourceState        src_state,
                           Image*               dst,
                           ResourceState        dst_state,
                           Filter               filter );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_clear_color_image,
                           const CommandBuffer* cmd,
                           Image*               image,
                           Vector4              color );

DECLARE_RENDERER_FUNCTION( void,
                           create_buffer,
                           const Device*     device,
                           const BufferInfo* info,
                           Buffer**          buffer );

DECLARE_RENDERER_FUNCTION( void,
                           destroy_buffer,
                           const Device* device,
                           Buffer*       buffer );

DECLARE_RENDERER_FUNCTION( void*,
                           map_memory,
                           const Device* device,
                           Buffer*       buffer );

DECLARE_RENDERER_FUNCTION( void,
                           unmap_memory,
                           const Device* device,
                           Buffer*       buffer );

DECLARE_RENDERER_FUNCTION( void,
                           cmd_draw_indexed_indirect,
                           const CommandBuffer* cmd,
                           const Buffer*        buffer,
                           u64                  offset,
                           u32                  draw_count,
                           u32                  stride );

DECLARE_RENDERER_FUNCTION( void,
                           create_sampler,
                           const Device*      device,
                           const SamplerInfo* info,
                           Sampler**          sampler );

DECLARE_RENDERER_FUNCTION( void,
                           destroy_sampler,
                           const Device* device,
                           Sampler*      sampler );

DECLARE_RENDERER_FUNCTION( void,
                           create_image,
                           const Device*    device,
                           const ImageInfo* info,
                           Image**          image );

DECLARE_RENDERER_FUNCTION( void,
                           destroy_image,
                           const Device* device,
                           Image*        image );

DECLARE_RENDERER_FUNCTION( void,
                           create_descriptor_set,
                           const Device*            device,
                           const DescriptorSetInfo* info,
                           DescriptorSet**          descriptor_set );

DECLARE_RENDERER_FUNCTION( void,
                           destroy_descriptor_set,
                           const Device*  device,
                           DescriptorSet* set );

DECLARE_RENDERER_FUNCTION( void,
                           update_descriptor_set,
                           const Device*          device,
                           DescriptorSet*         set,
                           u32                    count,
                           const DescriptorWrite* writes );

DECLARE_RENDERER_FUNCTION( void, init_ui, const UiInfo* info );
DECLARE_RENDERER_FUNCTION( void, ui_upload_resources, CommandBuffer* cmd );
DECLARE_RENDERER_FUNCTION( void, ui_destroy_upload_objects );
DECLARE_RENDERER_FUNCTION( void, shutdown_ui, const Device* device );
DECLARE_RENDERER_FUNCTION( void, ui_begin_frame, CommandBuffer* icmd );
DECLARE_RENDERER_FUNCTION( void, ui_end_frame, CommandBuffer* cmd );
DECLARE_RENDERER_FUNCTION( void*, get_imgui_texture_id, const Image* image );

DECLARE_RENDERER_FUNCTION( std::vector<char>,
                           read_shader,
                           const std::string& shader_name );
} // namespace fluent
