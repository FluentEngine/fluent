#ifdef D3D12_BACKEND

#define COBJMACROS
#include <tiny_image_format/tinyimageformat_apis.h>
#include <tiny_image_format/tinyimageformat_query.h>
#include "log/log.h"
#include "wsi/wsi.h"
#include "../renderer_private.h"
#include "d3d12_backend.h"

#ifdef FLUENT_DEBUG
#define D3D12_ASSERT( x )                                                      \
	do {                                                                       \
		HRESULT err = x;                                                       \
		if ( err != S_OK )                                                     \
		{                                                                      \
			FT_ERROR( "Detected D3D12 error: {}", err );                       \
			abort();                                                           \
		}                                                                      \
	} while ( 0 )
#else
#define D3D12_ASSERT( x ) x
#endif

FT_INLINE DXGI_FORMAT
to_dxgi_image_format( enum ft_format format )
{
	switch ( format )
	{
	case FT_FORMAT_B8G8R8A8_SRGB:
	{
		format = FT_FORMAT_B8G8R8A8_UNORM;
		break;
	}
	case FT_FORMAT_R8G8B8A8_SRGB:
	{
		format = FT_FORMAT_R8G8B8A8_UNORM;
		break;
	}
	}
	return ( DXGI_FORMAT ) TinyImageFormat_ToDXGI_FORMAT( format );
}

FT_INLINE DXGI_FORMAT
to_dxgi_format( enum ft_format format )
{
	return ( DXGI_FORMAT ) ( TinyImageFormat_ToDXGI_FORMAT( format ) );
}

FT_INLINE D3D12_COMMAND_LIST_TYPE
to_d3d12_command_list_type( enum ft_queue_type type )
{
	switch ( type )
	{
	case FT_QUEUE_TYPE_GRAPHICS: return D3D12_COMMAND_LIST_TYPE_DIRECT;
	case FT_QUEUE_TYPE_COMPUTE: return D3D12_COMMAND_LIST_TYPE_COMPUTE;
	case FT_QUEUE_TYPE_TRANSFER: return D3D12_COMMAND_LIST_TYPE_COPY;
	default: FT_ASSERT( false ); return ( D3D12_COMMAND_LIST_TYPE ) -1;
	}
}

FT_INLINE D3D12_RESOURCE_STATES
to_d3d12_resource_state( enum ft_resource_state state )
{
	switch ( state )
	{
	case FT_RESOURCE_STATE_UNDEFINED: return D3D12_RESOURCE_STATE_COMMON;
	case FT_RESOURCE_STATE_GENERAL:
		return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	case FT_RESOURCE_STATE_COLOR_ATTACHMENT:
		return D3D12_RESOURCE_STATE_RENDER_TARGET;
	case FT_RESOURCE_STATE_DEPTH_STENCIL_WRITE:
		return D3D12_RESOURCE_STATE_DEPTH_WRITE;
	case FT_RESOURCE_STATE_DEPTH_STENCIL_READ_ONLY:
		return D3D12_RESOURCE_STATE_DEPTH_READ;
	case FT_RESOURCE_STATE_SHADER_READ_ONLY:
		return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	case FT_RESOURCE_STATE_TRANSFER_SRC:
		return D3D12_RESOURCE_STATE_COPY_SOURCE;
	case FT_RESOURCE_STATE_TRANSFER_DST: return D3D12_RESOURCE_STATE_COPY_DEST;
	case FT_RESOURCE_STATE_PRESENT: return D3D12_RESOURCE_STATE_PRESENT;
	default: FT_ASSERT( false ); return ( D3D12_RESOURCE_STATES ) -1;
	}
}

FT_INLINE D3D12_FILL_MODE
to_d3d12_fill_mode( enum ft_polygon_mode mode )
{
	switch ( mode )
	{
	case FT_POLYGON_MODE_FILL: return D3D12_FILL_MODE_SOLID;
	case FT_POLYGON_MODE_LINE: return D3D12_FILL_MODE_WIREFRAME;
	default: FT_ASSERT( false ); return ( D3D12_FILL_MODE ) -1;
	}
}

FT_INLINE D3D12_CULL_MODE
to_d3d12_cull_mode( enum ft_cull_mode mode )
{
	switch ( mode )
	{
	case FT_CULL_MODE_NONE: return D3D12_CULL_MODE_NONE;
	case FT_CULL_MODE_BACK: return D3D12_CULL_MODE_BACK;
	case FT_CULL_MODE_FRONT: return D3D12_CULL_MODE_FRONT;
	default: FT_ASSERT( false ); return ( D3D12_CULL_MODE ) -1;
	}
}

FT_INLINE bool
to_d3d12_front_face( enum ft_front_face front_face )
{
	if ( front_face == FT_FRONT_FACE_COUNTER_CLOCKWISE )
	{
		return true;
	}
	else
	{
		return false;
	}
}

FT_INLINE D3D12_PRIMITIVE_TOPOLOGY_TYPE
to_d3d12_primitive_topology_type( enum ft_primitive_topology topology )
{
	switch ( topology )
	{
	case FT_PRIMITIVE_TOPOLOGY_POINT_LIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case FT_PRIMITIVE_TOPOLOGY_LINE_LIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case FT_PRIMITIVE_TOPOLOGY_LINE_STRIP:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case FT_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case FT_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	default: FT_ASSERT( false ); return ( D3D12_PRIMITIVE_TOPOLOGY_TYPE ) -1;
	}
}

FT_INLINE D3D12_PRIMITIVE_TOPOLOGY
to_d3d12_primitive_topology( enum ft_primitive_topology topology )
{
	switch ( topology )
	{
	case FT_PRIMITIVE_TOPOLOGY_POINT_LIST:
		return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	case FT_PRIMITIVE_TOPOLOGY_LINE_LIST:
		return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	case FT_PRIMITIVE_TOPOLOGY_LINE_STRIP:
		return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case FT_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case FT_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	default: FT_ASSERT( false ); return ( D3D12_PRIMITIVE_TOPOLOGY ) -1;
	}
}

FT_INLINE D3D12_HEAP_TYPE
to_d3d12_heap_type( enum ft_memory_usage usage )
{
	switch ( usage )
	{
	case FT_MEMORY_USAGE_GPU_ONLY: return D3D12_HEAP_TYPE_DEFAULT;
	case FT_MEMORY_USAGE_CPU_ONLY: return D3D12_HEAP_TYPE_UPLOAD;
	case FT_MEMORY_USAGE_CPU_TO_GPU: return D3D12_HEAP_TYPE_UPLOAD;
	case FT_MEMORY_USAGE_GPU_TO_CPU: return D3D12_HEAP_TYPE_READBACK;
	case FT_MEMORY_USAGE_CPU_COPY: return D3D12_HEAP_TYPE_UPLOAD;
	default: FT_ASSERT( false ); return ( D3D12_HEAP_TYPE ) -1;
	}
}

FT_INLINE D3D12_FILTER_TYPE
to_d3d12_filter_type( enum ft_filter filter )
{
	switch ( filter )
	{
	case FT_FILTER_NEAREST: return D3D12_FILTER_TYPE_POINT;
	case FT_FILTER_LINEAR: return D3D12_FILTER_TYPE_LINEAR;
	default: FT_ASSERT( false ); return ( D3D12_FILTER_TYPE ) -1;
	}
}

FT_INLINE D3D12_FILTER
to_d3d12_filter( enum ft_filter              min_filter,
                 enum ft_filter              mag_filter,
                 enum ft_sampler_mipmap_mode mip_map_mode,
                 bool                        anisotropy,
                 bool                        comparison_filter_enabled )
{
	if ( anisotropy )
		return ( comparison_filter_enabled ? D3D12_FILTER_COMPARISON_ANISOTROPIC
		                                   : D3D12_FILTER_ANISOTROPIC );

	int32_t filter =
	    ( min_filter << 4 ) | ( mag_filter << 2 ) | ( mip_map_mode );

	int32_t base_filter = comparison_filter_enabled
	                          ? D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT
	                          : D3D12_FILTER_MIN_MAG_MIP_POINT;

	return ( D3D12_FILTER ) ( base_filter + filter );
}

FT_INLINE D3D12_TEXTURE_ADDRESS_MODE
to_d3d12_address_mode( enum ft_sampler_address_mode mode )
{
	switch ( mode )
	{
	case FT_SAMPLER_ADDRESS_MODE_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	case FT_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	case FT_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case FT_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
		return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	default: FT_ASSERT( false ); return ( D3D12_TEXTURE_ADDRESS_MODE ) -1;
	}
}

FT_INLINE D3D12_COMPARISON_FUNC
to_d3d12_comparison_func( enum ft_compare_op op )
{
	switch ( op )
	{
	case FT_COMPARE_OP_NEVER: return D3D12_COMPARISON_FUNC_NEVER;
	case FT_COMPARE_OP_LESS: return D3D12_COMPARISON_FUNC_LESS;
	case FT_COMPARE_OP_EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
	case FT_COMPARE_OP_LESS_OR_EQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case FT_COMPARE_OP_GREATER: return D3D12_COMPARISON_FUNC_GREATER;
	case FT_COMPARE_OP_NOT_EQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case FT_COMPARE_OP_GREATER_OR_EQUAL:
		return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case FT_COMPARE_OP_ALWAYS: return D3D12_COMPARISON_FUNC_ALWAYS;
	default: FT_ASSERT( false ); return ( D3D12_COMPARISON_FUNC ) -1;
	}
}

FT_INLINE D3D12_DESCRIPTOR_RANGE_TYPE
to_d3d12_descriptor_range_type( enum ft_descriptor_type type )
{
	switch ( type )
	{
	case FT_DESCRIPTOR_TYPE_SAMPLER: return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	case FT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
	case FT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
	case FT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
		return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	default: return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	}
}

FT_INLINE D3D12_SHADER_VISIBILITY
to_d3d12_shader_visibility( enum ft_shader_stage stage )
{
	switch ( stage )
	{
	case FT_SHADER_STAGE_VERTEX: return D3D12_SHADER_VISIBILITY_VERTEX;
	case FT_SHADER_STAGE_TESSELLATION_CONTROL:
		return D3D12_SHADER_VISIBILITY_HULL;
	case FT_SHADER_STAGE_TESSELLATION_EVALUATION:
		return D3D12_SHADER_VISIBILITY_DOMAIN;
	case FT_SHADER_STAGE_GEOMETRY: return D3D12_SHADER_VISIBILITY_GEOMETRY;
	case FT_SHADER_STAGE_FRAGMENT: return D3D12_SHADER_VISIBILITY_PIXEL;
	default: FT_ASSERT( false ); return ( D3D12_SHADER_VISIBILITY ) -1;
	}
}

void
d3d12_destroy_renderer_backend( struct ft_renderer_backend* ibackend )
{
	FT_FROM_HANDLE( backend, ibackend, D3D12RendererBackend );

#ifdef FLUENT_DEBUG
	ID3D12Debug_Release( backend->debug_controller );
#endif
	IDXGIFactory4_Release( backend->factory );
}

void
d3d12_create_device( const struct ft_renderer_backend* ibackend,
                     const struct ft_device_info*      info,
                     struct ft_device**                p )
{
}

void
d3d12_destroy_device( struct ft_device* idevice )
{
}

void
d3d12_create_queue( const struct ft_device*     idevice,
                    const struct ft_queue_info* info,
                    struct ft_queue**           p )
{
}

void
d3d12_destroy_queue( struct ft_queue* iqueue )
{
}

void
d3d12_create_command_pool( const struct ft_device*            idevice,
                           const struct ft_command_pool_info* info,
                           struct ft_command_pool**           p )
{
}

void
d3d12_destroy_command_pool( const struct ft_device* idevice,
                            struct ft_command_pool* icommand_pool )
{
}

void
d3d12_create_command_buffers( const struct ft_device*       idevice,
                              const struct ft_command_pool* icommand_pool,
                              uint32_t                      count,
                              struct ft_command_buffer**    icommand_buffers )
{
}

void
d3d12_free_command_buffers( const struct ft_device*       idevice,
                            const struct ft_command_pool* icommand_pool,
                            uint32_t                      count,
                            struct ft_command_buffer**    icommand_buffers )
{
}

void
d3d12_destroy_command_buffers( const struct ft_device*       idevice,
                               const struct ft_command_pool* icommand_pool,
                               uint32_t                      count,
                               struct ft_command_buffer**    icommand_buffers )
{
}

void
d3d12_create_semaphore( const struct ft_device* idevice,
                        struct ft_semaphore**   p )
{
}

void
d3d12_destroy_semaphore( const struct ft_device* idevice,
                         struct ft_semaphore*    isemaphore )
{
}

void
d3d12_create_fence( const struct ft_device* idevice, struct ft_fence** p )
{
}

void
d3d12_destroy_fence( const struct ft_device* idevice, struct ft_fence* ifence )
{
}

void
d3d12_queue_wait_idle( const struct ft_queue* iqueue )
{
}

void
d3d12_queue_submit( const struct ft_queue*             iqueue,
                    const struct ft_queue_submit_info* info )
{
}

void
d3d12_immediate_submit( const struct ft_queue*    iqueue,
                        struct ft_command_buffer* icmd )
{
}

void
d3d12_queue_present( const struct ft_queue*              iqueue,
                     const struct ft_queue_present_info* info )
{
}

void
d3d12_wait_for_fences( const struct ft_device* idevice,
                       uint32_t                count,
                       struct ft_fence**       ifences )
{
}

void
d3d12_reset_fences( const struct ft_device* idevice,
                    uint32_t                count,
                    struct ft_fence**       ifences )
{
}

void
d3d12_create_swapchain( const struct ft_device*         idevice,
                        const struct ft_swapchain_info* info,
                        struct ft_swapchain**           p )
{
}

void
d3d12_resize_swapchain( const struct ft_device* idevice,
                        struct ft_swapchain*    iswapchain,
                        uint32_t                width,
                        uint32_t                height )
{
}

void
d3d12_destroy_swapchain( const struct ft_device* idevice,
                         struct ft_swapchain*    iswapchain )
{
}

void
d3d12_begin_command_buffer( const struct ft_command_buffer* icmd )
{
}

void
d3d12_end_command_buffer( const struct ft_command_buffer* icmd )
{
}

void
d3d12_acquire_next_image( const struct ft_device*    idevice,
                          const struct ft_swapchain* iswapchain,
                          const struct ft_semaphore* isemaphore,
                          const struct ft_fence*     ifence,
                          uint32_t*                  image_index )
{
}

void
d3d12_create_shader( const struct ft_device* idevice,
                     struct ft_shader_info*  info,
                     struct ft_shader**      p )
{
}

void
d3d12_destroy_shader( const struct ft_device* idevice,
                      struct ft_shader*       ishader )
{
}

void
d3d12_create_descriptor_set_layout( const struct ft_device*           idevice,
                                    struct ft_shader*                 ishader,
                                    struct ft_descriptor_set_layout** p ) {};

void
d3d12_destroy_descriptor_set_layout( const struct ft_device*          idevice,
                                     struct ft_descriptor_set_layout* ilayout )
{
}

void
d3d12_create_compute_pipeline( const struct ft_device*        idevice,
                               const struct ft_pipeline_info* info,
                               struct ft_pipeline**           p )
{
}

void
d3d12_create_graphics_pipeline( const struct ft_device*        idevice,
                                const struct ft_pipeline_info* info,
                                struct ft_pipeline**           p )
{
}

void
d3d12_destroy_pipeline( const struct ft_device* idevice,
                        struct ft_pipeline*     ipipeline )
{
}

void
d3d12_cmd_begin_render_pass( const struct ft_command_buffer*         icmd,
                             const struct ft_render_pass_begin_info* info )
{
}

void
d3d12_cmd_end_render_pass( const struct ft_command_buffer* icmd )
{
}

void
d3d12_cmd_barrier( const struct ft_command_buffer* icmd,
                   uint32_t                        memory_barriers_count,
                   const struct ft_memory_barrier* memory_barrier,
                   uint32_t                        buffer_barriers_count,
                   const struct ft_buffer_barrier* buffer_barriers,
                   uint32_t                        image_barriers_count,
                   const struct ft_image_barrier*  image_barriers ) {};

void
d3d12_cmd_set_scissor( const struct ft_command_buffer* icmd,
                       int32_t                         x,
                       int32_t                         y,
                       uint32_t                        width,
                       uint32_t                        height )
{
}

void
d3d12_cmd_set_viewport( const struct ft_command_buffer* icmd,
                        float                           x,
                        float                           y,
                        float                           width,
                        float                           height,
                        float                           min_depth,
                        float                           max_depth )
{
}

void
d3d12_cmd_bind_pipeline( const struct ft_command_buffer* icmd,
                         const struct ft_pipeline*       ipipeline )
{
}

void
d3d12_cmd_draw( const struct ft_command_buffer* icmd,
                uint32_t                        vertex_count,
                uint32_t                        instance_count,
                uint32_t                        first_vertex,
                uint32_t                        first_instance )
{
}

void
d3d12_cmd_draw_indexed( const struct ft_command_buffer* icmd,
                        uint32_t                        index_count,
                        uint32_t                        instance_count,
                        uint32_t                        first_index,
                        int32_t                         vertex_offset,
                        uint32_t                        first_instance )
{
}

void
d3d12_cmd_bind_vertex_buffer( const struct ft_command_buffer* icmd,
                              const struct ft_buffer*         ibuffer,
                              const uint64_t                  offset )
{
}

void
d3d12_cmd_bind_index_buffer_uint16_t( const struct ft_command_buffer* cmd,
                                      const struct ft_buffer*         buffer,
                                      const uint64_t                  offset )
{
}

void
d3d12_cmd_bind_index_buffer_uint32_t( const struct ft_command_buffer* cmd,
                                      const struct ft_buffer*         buffer,
                                      uint64_t                        offset )
{
}

void
d3d12_cmd_copy_buffer( const struct ft_command_buffer* icmd,
                       const struct ft_buffer*         isrc,
                       uint64_t                        src_offset,
                       struct ft_buffer*               idst,
                       uint64_t                        dst_offset,
                       uint64_t                        size )
{
}

void
d3d12_cmd_copy_buffer_to_image( const struct ft_command_buffer* icmd,
                                const struct ft_buffer*         isrc,
                                uint64_t                        src_offset,
                                struct ft_image*                idst )
{
}

void
d3d12_cmd_dispatch( const struct ft_command_buffer* cmd,
                    uint32_t                        group_count_x,
                    uint32_t                        group_count_y,
                    uint32_t                        group_count_z )
{
}

void
d3d12_cmd_push_constants( const struct ft_command_buffer* cmd,
                          const struct ft_pipeline*       pipeline,
                          uint64_t                        offset,
                          uint64_t                        size,
                          const void*                     data )
{
}

void
d3d12_cmd_blit_image( const struct ft_command_buffer* cmd,
                      const struct ft_image*          src,
                      enum ft_resource_state          src_state,
                      struct ft_image*                dst,
                      enum ft_resource_state          dst_state,
                      enum ft_filter                  filter )
{
}

void
d3d12_cmd_clear_color_image( const struct ft_command_buffer* cmd,
                             struct ft_image*                image,
                             float                           color[ 4 ] )
{
}

void
d3d12_cmd_draw_indexed_indirect( const struct ft_command_buffer* cmd,
                                 const struct ft_buffer*         buffer,
                                 uint64_t                        offset,
                                 uint32_t                        draw_count,
                                 uint32_t                        stride )
{
}

void
d3d12_cmd_bind_descriptor_set( const struct ft_command_buffer* cmd,
                               uint32_t                        first_set,
                               const struct ft_descriptor_set* set,
                               const struct ft_pipeline*       pipeline )
{
}

void
d3d12_create_buffer( const struct ft_device*      idevice,
                     const struct ft_buffer_info* info,
                     struct ft_buffer**           p )
{
}

void
d3d12_destroy_buffer( const struct ft_device* idevice,
                      struct ft_buffer*       ibuffer )
{
}

void*
d3d12_map_memory( const struct ft_device* idevice, struct ft_buffer* ibuffer )
{
}

void
d3d12_unmap_memory( const struct ft_device* idevice, struct ft_buffer* ibuffer )
{
}

void
d3d12_create_sampler( const struct ft_device*       idevice,
                      const struct ft_sampler_info* info,
                      struct ft_sampler**           p )
{
}

void
d3d12_destroy_sampler( const struct ft_device* device,
                       struct ft_sampler*      isampler )
{
}

void
d3d12_create_image( const struct ft_device*     idevice,
                    const struct ft_image_info* info,
                    struct ft_image**           p )
{
}

void
d3d12_destroy_image( const struct ft_device* device, struct ft_image* iimage )
{
}

void
d3d12_create_descriptor_set( const struct ft_device*              idevice,
                             const struct ft_descriptor_set_info* info,
                             struct ft_descriptor_set**           p )
{
}

void
d3d12_destroy_descriptor_set( const struct ft_device*   idevice,
                              struct ft_descriptor_set* iset )
{
}

void
d3d12_update_descriptor_set( const struct ft_device*           idevice,
                             struct ft_descriptor_set*         iset,
                             uint32_t                          count,
                             const struct ft_descriptor_write* writes )
{
}

void
d3d12_create_renderer_backend( const struct ft_renderer_backend_info* info,
                               struct ft_renderer_backend**           p )
{
	destroy_renderer_backend_impl       = d3d12_destroy_renderer_backend;
	create_device_impl                  = d3d12_create_device;
	destroy_device_impl                 = d3d12_destroy_device;
	create_queue_impl                   = d3d12_create_queue;
	destroy_queue_impl                  = d3d12_destroy_queue;
	queue_wait_idle_impl                = d3d12_queue_wait_idle;
	queue_submit_impl                   = d3d12_queue_submit;
	immediate_submit_impl               = d3d12_immediate_submit;
	queue_present_impl                  = d3d12_queue_present;
	create_semaphore_impl               = d3d12_create_semaphore;
	destroy_semaphore_impl              = d3d12_destroy_semaphore;
	create_fence_impl                   = d3d12_create_fence;
	destroy_fence_impl                  = d3d12_destroy_fence;
	wait_for_fences_impl                = d3d12_wait_for_fences;
	reset_fences_impl                   = d3d12_reset_fences;
	create_swapchain_impl               = d3d12_create_swapchain;
	resize_swapchain_impl               = d3d12_resize_swapchain;
	destroy_swapchain_impl              = d3d12_destroy_swapchain;
	create_command_pool_impl            = d3d12_create_command_pool;
	destroy_command_pool_impl           = d3d12_destroy_command_pool;
	create_command_buffers_impl         = d3d12_create_command_buffers;
	free_command_buffers_impl           = d3d12_free_command_buffers;
	destroy_command_buffers_impl        = d3d12_destroy_command_buffers;
	begin_command_buffer_impl           = d3d12_begin_command_buffer;
	end_command_buffer_impl             = d3d12_end_command_buffer;
	acquire_next_image_impl             = d3d12_acquire_next_image;
	create_shader_impl                  = d3d12_create_shader;
	destroy_shader_impl                 = d3d12_destroy_shader;
	create_descriptor_set_layout_impl   = d3d12_create_descriptor_set_layout;
	destroy_descriptor_set_layout_impl  = d3d12_destroy_descriptor_set_layout;
	create_compute_pipeline_impl        = d3d12_create_compute_pipeline;
	create_graphics_pipeline_impl       = d3d12_create_graphics_pipeline;
	destroy_pipeline_impl               = d3d12_destroy_pipeline;
	create_buffer_impl                  = d3d12_create_buffer;
	destroy_buffer_impl                 = d3d12_destroy_buffer;
	map_memory_impl                     = d3d12_map_memory;
	unmap_memory_impl                   = d3d12_unmap_memory;
	create_sampler_impl                 = d3d12_create_sampler;
	destroy_sampler_impl                = d3d12_destroy_sampler;
	create_image_impl                   = d3d12_create_image;
	destroy_image_impl                  = d3d12_destroy_image;
	create_descriptor_set_impl          = d3d12_create_descriptor_set;
	destroy_descriptor_set_impl         = d3d12_destroy_descriptor_set;
	update_descriptor_set_impl          = d3d12_update_descriptor_set;
	cmd_begin_render_pass_impl          = d3d12_cmd_begin_render_pass;
	cmd_end_render_pass_impl            = d3d12_cmd_end_render_pass;
	cmd_barrier_impl                    = d3d12_cmd_barrier;
	cmd_set_scissor_impl                = d3d12_cmd_set_scissor;
	cmd_set_viewport_impl               = d3d12_cmd_set_viewport;
	cmd_bind_pipeline_impl              = d3d12_cmd_bind_pipeline;
	cmd_draw_impl                       = d3d12_cmd_draw;
	cmd_draw_indexed_impl               = d3d12_cmd_draw_indexed;
	cmd_bind_vertex_buffer_impl         = d3d12_cmd_bind_vertex_buffer;
	cmd_bind_index_buffer_uint16_t_impl = d3d12_cmd_bind_index_buffer_uint16_t;
	cmd_bind_index_buffer_uint32_t_impl = d3d12_cmd_bind_index_buffer_uint32_t;
	cmd_copy_buffer_impl                = d3d12_cmd_copy_buffer;
	cmd_copy_buffer_to_image_impl       = d3d12_cmd_copy_buffer_to_image;
	cmd_bind_descriptor_set_impl        = d3d12_cmd_bind_descriptor_set;
	cmd_dispatch_impl                   = d3d12_cmd_dispatch;
	cmd_push_constants_impl             = d3d12_cmd_push_constants;
	cmd_clear_color_image_impl          = d3d12_cmd_clear_color_image;
	cmd_draw_indexed_indirect_impl      = d3d12_cmd_draw_indexed_indirect;

	FT_INIT_INTERNAL( backend, *p, D3D12RendererBackend );

	UINT flags = 0;

#ifdef FLUENT_DEBUG
	D3D12_ASSERT( D3D12GetDebugInterface( &IID_ID3D12Debug,
	                                      &backend->debug_controller ) );
	ID3D12Debug_EnableDebugLayer( backend->debug_controller );
	flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	D3D12_ASSERT(
	    CreateDXGIFactory2( flags, &IID_IDXGIFactory4, &backend->factory ) );
}
#endif
