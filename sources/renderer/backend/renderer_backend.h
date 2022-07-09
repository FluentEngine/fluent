#pragma once

#include "base/base.h"
#include "renderer/backend/renderer_enums.h"

#define FT_MAX_DEVICE_COUNT                    1
#define FT_MAX_ATTACHMENTS_COUNT               10
#define FT_MAX_PUSH_CONSTANT_RANGE             128
#define FT_MAX_VERTEX_BINDING_COUNT            15
#define FT_MAX_VERTEX_ATTRIBUTE_COUNT          15
#define FT_MAX_DESCRIPTOR_BINDING_COUNT        15
#define FT_MAX_SET_COUNT                       10
#define FT_RESOURCE_LOADER_STAGING_BUFFER_SIZE 25 * 1024 * 1024 * 8
#define FT_MAX_BINDING_NAME_LENGTH             20

struct ft_wsi_info;

// Forward declares
struct ft_window;
struct ft_buffer;
struct ft_image;
struct ft_queue;
struct ft_command_buffer;
struct ft_command_pool;

struct ft_renderer_backend_info
{
	enum ft_renderer_api api;
	struct ft_wsi_info*  wsi_info;
};

struct ft_renderer_backend
{
	enum ft_renderer_api api;
	ft_handle            handle;
};

struct ft_device_info
{
	struct ft_renderer_backend* backend;
};

struct ft_device
{
	enum ft_renderer_api api;
	ft_handle            handle;
};

struct ft_command_pool_info
{
	struct ft_queue* queue;
};

struct ft_command_pool
{
	struct ft_queue* queue;
	ft_handle        handle;
};

struct ft_command_buffer
{
	struct ft_queue* queue;
	ft_handle        handle;
};

struct ft_queue_info
{
	enum ft_queue_type queue_type;
};

struct ft_queue
{
	uint32_t           family_index;
	enum ft_queue_type type;
	ft_handle          handle;
};

struct ft_semaphore
{
	ft_handle handle;
};

struct ft_fence
{
	ft_handle handle;
};

struct ft_sampler_info
{
	enum ft_filter               mag_filter;
	enum ft_filter               min_filter;
	enum ft_sampler_mipmap_mode  mipmap_mode;
	enum ft_sampler_address_mode address_mode_u;
	enum ft_sampler_address_mode address_mode_v;
	enum ft_sampler_address_mode address_mode_w;
	float                        mip_lod_bias;
	bool                         anisotropy_enable;
	float                        max_anisotropy;
	bool                         compare_enable;
	enum ft_compare_op           compare_op;
	float                        min_lod;
	float                        max_lod;
};

struct ft_sampler
{
	ft_handle handle;
};

struct ft_image_info
{
	uint32_t                width;
	uint32_t                height;
	uint32_t                depth;
	enum ft_format          format;
	uint32_t                sample_count;
	uint32_t                layer_count;
	uint32_t                mip_levels;
	enum ft_descriptor_type descriptor_type;
};

struct ft_image
{
	uint32_t                width;
	uint32_t                height;
	uint32_t                depth;
	enum ft_format          format;
	uint32_t                sample_count;
	uint32_t                mip_level_count;
	uint32_t                layer_count;
	enum ft_descriptor_type descriptor_type;
	ft_handle               handle;
};

struct ft_buffer_info
{
	uint64_t                size;
	enum ft_descriptor_type descriptor_type;
	enum ft_memory_usage    memory_usage;
};

struct ft_buffer
{
	uint64_t                size;
	enum ft_resource_state  resource_state;
	enum ft_descriptor_type descriptor_type;
	enum ft_memory_usage    memory_usage;
	void*                   mapped_memory;
	ft_handle               handle;
};

struct ft_swapchain_info
{
	struct ft_queue*    queue;
	uint32_t            width;
	uint32_t            height;
	enum ft_format      format;
	bool                vsync;
	uint32_t            min_image_count;
	struct ft_wsi_info* wsi_info;
};

struct ft_swapchain
{
	uint32_t          min_image_count;
	uint32_t          image_count;
	uint32_t          width;
	uint32_t          height;
	enum ft_format    format;
	struct ft_image** images;
	struct ft_queue*  queue;
	bool              vsync;
	ft_handle         handle;
};

struct ft_queue_submit_info
{
	uint32_t                   wait_semaphore_count;
	struct ft_semaphore**      wait_semaphores;
	uint32_t                   command_buffer_count;
	struct ft_command_buffer** command_buffers;
	uint32_t                   signal_semaphore_count;
	struct ft_semaphore**      signal_semaphores;
	struct ft_fence*           signal_fence;
};

struct ft_queue_present_info
{
	uint32_t              wait_semaphore_count;
	struct ft_semaphore** wait_semaphores;
	struct ft_swapchain*  swapchain;
	uint32_t              image_index;
};

typedef float ft_color_clear_value[ 4 ];

struct ft_depth_stencil_clear_value
{
	float    depth;
	uint32_t stencil;
};

struct ft_clear_value
{
	ft_color_clear_value                color;
	struct ft_depth_stencil_clear_value depth_stencil;
};

struct ft_attachment_info
{
	const struct ft_image*     image;
	enum ft_attachment_load_op load_op;
	struct ft_clear_value      clear_value;
};

struct ft_render_pass_begin_info
{
	uint32_t                  width;
	uint32_t                  height;
	uint32_t                  color_attachment_count;
	struct ft_attachment_info color_attachments[ FT_MAX_ATTACHMENTS_COUNT ];
	struct ft_attachment_info depth_attachment;
};

// TODO:
struct ft_memory_barrier
{
	void* allocation;
};

struct ft_buffer_barrier
{
	enum ft_resource_state old_state;
	enum ft_resource_state new_state;
	struct ft_buffer*      buffer;
	struct ft_queue*       src_queue;
	struct ft_queue*       dst_queue;
	uint32_t               size;
	uint32_t               offset;
};

struct ft_image_barrier
{
	enum ft_resource_state old_state;
	enum ft_resource_state new_state;
	const struct ft_image* image;
	const struct ft_queue* src_queue;
	const struct ft_queue* dst_queue;
};

struct ft_shader_module_info
{
	uint32_t    bytecode_size;
	const void* bytecode;
};

struct ft_shader_info
{
	struct ft_shader_module_info compute;
	struct ft_shader_module_info vertex;
	struct ft_shader_module_info tessellation_control;
	struct ft_shader_module_info tessellation_evaluation;
	struct ft_shader_module_info geometry;
	struct ft_shader_module_info fragment;
};

struct ft_binding
{
	uint32_t                set;
	uint32_t                binding;
	uint32_t                descriptor_count;
	enum ft_descriptor_type descriptor_type;
	enum ft_shader_stage    stage;
};

typedef struct ft_binding* ft_bindings;

struct ft_binding_map_item
{
	char     name[ FT_MAX_BINDING_NAME_LENGTH ];
	uint32_t value;
};

struct ft_reflection_data
{
	uint32_t        binding_count;
	ft_bindings     bindings;
	struct hashmap* binding_map;
};

struct ft_shader
{
	struct ft_reflection_data reflect_data;
	ft_handle                 handle;
};

struct ft_descriptor_set_layout
{
	struct ft_reflection_data reflection_data;
	ft_handle                 handle;
};

struct ft_vertex_binding_info
{
	uint32_t                  binding;
	uint32_t                  stride;
	enum ft_vertex_input_rate input_rate;
};

struct ft_vertex_attribute_info
{
	uint32_t       location;
	uint32_t       binding;
	enum ft_format format;
	uint32_t       offset;
};

struct ft_vertex_layout
{
	uint32_t                      binding_info_count;
	struct ft_vertex_binding_info binding_infos[ FT_MAX_VERTEX_BINDING_COUNT ];
	uint32_t                      attribute_info_count;
	struct ft_vertex_attribute_info
	    attribute_infos[ FT_MAX_VERTEX_ATTRIBUTE_COUNT ];
};

struct ft_rasterizer_state_info
{
	enum ft_cull_mode    cull_mode;
	enum ft_front_face   front_face;
	enum ft_polygon_mode polygon_mode;
};

struct ft_depth_state_info
{
	bool               depth_test;
	bool               depth_write;
	enum ft_compare_op compare_op;
};

struct ft_blend_attachment_state
{
	enum ft_blend_factor src_factor;
	enum ft_blend_factor dst_factor;
	enum ft_blend_factor src_alpha_factor;
	enum ft_blend_factor dst_alpha_factor;
	enum ft_blend_op     op;
	enum ft_blend_op     alpha_op;
};

struct ft_blend_state_info
{
	struct ft_blend_attachment_state
	    attachment_states[ FT_MAX_ATTACHMENTS_COUNT ];
};

struct ft_pipeline_info
{
	enum ft_pipeline_type            type;
	struct ft_vertex_layout          vertex_layout;
	struct ft_rasterizer_state_info  rasterizer_info;
	enum ft_primitive_topology       topology;
	struct ft_depth_state_info       depth_state_info;
	struct ft_shader*                shader;
	struct ft_descriptor_set_layout* descriptor_set_layout;
	uint32_t                         sample_count;
	uint32_t                         color_attachment_count;
	enum ft_format color_attachment_formats[ FT_MAX_ATTACHMENTS_COUNT ];
	enum ft_format depth_stencil_format;
	struct ft_blend_state_info blend_state_info;
};

struct ft_pipeline
{
	enum ft_pipeline_type type;
	ft_handle             handle;
};

struct ft_descriptor_set_info
{
	uint32_t                         set;
	struct ft_descriptor_set_layout* descriptor_set_layout;
};

struct ft_descriptor_set
{
	struct ft_descriptor_set_layout* layout;
	ft_handle                        handle;
};

struct ft_buffer_descriptor
{
	struct ft_buffer* buffer;
	uint64_t          offset;
	uint64_t          range;
};

struct ft_image_descriptor
{
	struct ft_image*       image;
	enum ft_resource_state resource_state;
};

struct ft_sampler_descriptor
{
	struct ft_sampler* sampler;
};

struct ft_descriptor_write
{
	uint32_t                      descriptor_count;
	const char*                   descriptor_name;
	struct ft_sampler_descriptor* sampler_descriptors;
	struct ft_image_descriptor*   image_descriptors;
	struct ft_buffer_descriptor*  buffer_descriptors;
};

struct ft_buffer_image_copy
{
	uint64_t buffer_offset;
	uint32_t width;
	uint32_t height;
	uint32_t mip_level;
};

struct ft_upload_image_info
{
	const void* data;
	uint32_t    width;
	uint32_t    height;
	uint32_t    mip_level;
};

FT_INLINE bool
ft_format_has_depth_aspect( enum ft_format format )
{
	switch ( format )
	{
	case FT_FORMAT_D16_UNORM:
	case FT_FORMAT_D16_UNORMS8_UINT:
	case FT_FORMAT_D24_UNORMS8_UINT:
	case FT_FORMAT_D32_SFLOAT:
	case FT_FORMAT_X8D24_UNORM:
	case FT_FORMAT_D32_SFLOATS8_UINT: return true;
	default: return false;
	}
}

FT_INLINE bool
ft_format_has_stencil_aspect( enum ft_format format )
{
	switch ( format )
	{
	case FT_FORMAT_D16_UNORMS8_UINT:
	case FT_FORMAT_D24_UNORMS8_UINT:
	case FT_FORMAT_D32_SFLOATS8_UINT:
	case FT_FORMAT_S8_UINT: return true;
	default: return false;
	}
}

FT_INLINE bool
ft_is_srgb( enum ft_format format )
{
	switch ( format )
	{
	case FT_FORMAT_B8G8R8A8_SRGB:
	case FT_FORMAT_B8G8R8_SRGB:
	case FT_FORMAT_DXBC2_SRGB:
	case FT_FORMAT_DXBC3_SRGB:
	case FT_FORMAT_DXBC7_SRGB:
	case FT_FORMAT_R8G8B8A8_SRGB:
	case FT_FORMAT_R8G8B8_SRGB:
	case FT_FORMAT_R8G8_SRGB:
	case FT_FORMAT_R8_SRGB: return true;
	default: return false;
	}
}

FT_INLINE uint32_t
ft_format_size_bytes( enum ft_format const format )
{
	return TinyImageFormat_BitSizeOfBlock( ( TinyImageFormat ) format ) / 8;
}

FT_API void
ft_create_renderer_backend( const struct ft_renderer_backend_info* info,
                            struct ft_renderer_backend**           backend );

FT_API void
ft_destroy_renderer_backend( struct ft_renderer_backend* backend );

FT_API void
ft_create_device( const struct ft_renderer_backend* backend,
                  const struct ft_device_info*      info,
                  struct ft_device**                device );

FT_API void
ft_destroy_device( struct ft_device* device );

FT_API void
ft_create_queue( const struct ft_device*     device,
                 const struct ft_queue_info* info,
                 struct ft_queue**           queue );

FT_API void
ft_destroy_queue( struct ft_queue* queue );

FT_API void
ft_queue_wait_idle( const struct ft_queue* queue );

FT_API void
ft_queue_submit( const struct ft_queue*             queue,
                 const struct ft_queue_submit_info* info );

FT_API void
ft_immediate_submit( const struct ft_queue*    queue,
                     struct ft_command_buffer* cmd );

FT_API void
ft_queue_present( const struct ft_queue*              queue,
                  const struct ft_queue_present_info* info );

FT_API void
ft_create_semaphore( const struct ft_device* device,
                     struct ft_semaphore**   semaphore );

FT_API void
ft_destroy_semaphore( const struct ft_device* device,
                      struct ft_semaphore*    semaphore );

FT_API void
ft_create_fence( const struct ft_device* device, struct ft_fence** fence );

FT_API void
ft_destroy_fence( const struct ft_device* device, struct ft_fence* fence );

FT_API void
ft_wait_for_fences( const struct ft_device* device,
                    uint32_t                count,
                    struct ft_fence**       fences );

FT_API void
ft_reset_fences( const struct ft_device* device,
                 uint32_t                count,
                 struct ft_fence**       fences );

FT_API void
ft_create_swapchain( const struct ft_device*         device,
                     const struct ft_swapchain_info* info,
                     struct ft_swapchain**           swapchain );

FT_API void
ft_resize_swapchain( const struct ft_device* device,
                     struct ft_swapchain*    swapchain,
                     uint32_t                width,
                     uint32_t                height );

FT_API void
ft_destroy_swapchain( const struct ft_device* device,
                      struct ft_swapchain*    swapchain );

FT_API void
ft_create_command_pool( const struct ft_device*            device,
                        const struct ft_command_pool_info* info,
                        struct ft_command_pool**           command_pool );

FT_API void
ft_destroy_command_pool( const struct ft_device* device,
                         struct ft_command_pool* command_pool );

FT_API void
ft_create_command_buffers( const struct ft_device*       device,
                           const struct ft_command_pool* command_pool,
                           uint32_t                      count,
                           struct ft_command_buffer**    command_buffers );

FT_API void
ft_free_command_buffers( const struct ft_device*       device,
                         const struct ft_command_pool* command_pool,
                         uint32_t                      count,
                         struct ft_command_buffer**    command_buffers );

FT_API void
ft_destroy_command_buffers( const struct ft_device*       device,
                            const struct ft_command_pool* command_pool,
                            uint32_t                      count,
                            struct ft_command_buffer**    command_buffers );

FT_API void
ft_begin_command_buffer( const struct ft_command_buffer* cmd );

FT_API void
ft_end_command_buffer( const struct ft_command_buffer* cmd );

FT_API void
ft_acquire_next_image( const struct ft_device*    device,
                       const struct ft_swapchain* swapchain,
                       const struct ft_semaphore* semaphore,
                       const struct ft_fence*     fence,
                       uint32_t*                  image_index );

FT_API void
ft_create_shader( const struct ft_device* device,
                  struct ft_shader_info*  info,
                  struct ft_shader**      shader );

FT_API void
ft_destroy_shader( const struct ft_device* device, struct ft_shader* shader );

FT_API void
ft_create_descriptor_set_layout(
    const struct ft_device*           device,
    struct ft_shader*                 shader,
    struct ft_descriptor_set_layout** descriptor_set_layout );

FT_API void
ft_destroy_descriptor_set_layout( const struct ft_device*          device,
                                  struct ft_descriptor_set_layout* layout );

FT_API void
ft_create_pipeline( const struct ft_device*        device,
                    const struct ft_pipeline_info* info,
                    struct ft_pipeline**           pipeline );

FT_API void
ft_destroy_pipeline( const struct ft_device* device,
                     struct ft_pipeline*     pipeline );

FT_API void
ft_cmd_begin_render_pass( const struct ft_command_buffer*         cmd,
                          const struct ft_render_pass_begin_info* info );

FT_API void
ft_cmd_end_render_pass( const struct ft_command_buffer* cmd );

FT_API void
ft_cmd_barrier( const struct ft_command_buffer* cmd,
                uint32_t                        memory_barriers_count,
                const struct ft_memory_barrier* memory_barrier,
                uint32_t                        buffer_barriers_count,
                const struct ft_buffer_barrier* buffer_barriers,
                uint32_t                        image_barriers_count,
                const struct ft_image_barrier*  image_barriers );

FT_API void
ft_cmd_set_scissor( const struct ft_command_buffer* cmd,
                    int32_t                         x,
                    int32_t                         y,
                    uint32_t                        width,
                    uint32_t                        height );

FT_API void
ft_cmd_set_viewport( const struct ft_command_buffer* cmd,
                     float                           x,
                     float                           y,
                     float                           width,
                     float                           height,
                     float                           min_depth,
                     float                           max_depth );

FT_API void
ft_cmd_bind_pipeline( const struct ft_command_buffer* cmd,
                      const struct ft_pipeline*       pipeline );

FT_API void
ft_cmd_draw( const struct ft_command_buffer* cmd,
             uint32_t                        vertex_count,
             uint32_t                        instance_count,
             uint32_t                        first_vertex,
             uint32_t                        first_instance );

FT_API void
ft_cmd_draw_indexed( const struct ft_command_buffer* cmd,
                     uint32_t                        index_count,
                     uint32_t                        instance_count,
                     uint32_t                        first_index,
                     int32_t                         vertex_offset,
                     uint32_t                        first_instance );

FT_API void
ft_cmd_bind_vertex_buffer( const struct ft_command_buffer* cmd,
                           const struct ft_buffer*         buffer,
                           const uint64_t                  offset );

FT_API void
ft_cmd_bind_index_buffer( const struct ft_command_buffer* cmd,
                          const struct ft_buffer*         buffer,
                          const uint64_t                  offset,
                          enum ft_index_type              index_type );

FT_API void
ft_cmd_copy_buffer( const struct ft_command_buffer* cmd,
                    const struct ft_buffer*         src,
                    uint64_t                        src_offset,
                    struct ft_buffer*               dst,
                    uint64_t                        dst_offset,
                    uint64_t                        size );

FT_API void
ft_cmd_copy_buffer_to_image( const struct ft_command_buffer*    cmd,
                             const struct ft_buffer*            src,
                             struct ft_image*                   dst,
                             const struct ft_buffer_image_copy* copy );

FT_API void
ft_cmd_bind_descriptor_set( const struct ft_command_buffer* cmd,
                            uint32_t                        first_set,
                            const struct ft_descriptor_set* set,
                            const struct ft_pipeline*       pipeline );

FT_API void
ft_cmd_dispatch( const struct ft_command_buffer* cmd,
                 uint32_t                        group_count_x,
                 uint32_t                        group_count_y,
                 uint32_t                        group_count_z );

FT_API void
ft_cmd_push_constants( const struct ft_command_buffer* cmd,
                       const struct ft_pipeline*       pipeline,
                       uint32_t                        offset,
                       uint32_t                        size,
                       const void*                     data );

FT_API void
ft_create_buffer( const struct ft_device*      device,
                  const struct ft_buffer_info* info,
                  struct ft_buffer**           buffer );

FT_API void
ft_destroy_buffer( const struct ft_device* device, struct ft_buffer* buffer );

FT_API void*
ft_map_memory( const struct ft_device* device, struct ft_buffer* buffer );

FT_API void
ft_unmap_memory( const struct ft_device* device, struct ft_buffer* buffer );

FT_API void
ft_cmd_draw_indexed_indirect( const struct ft_command_buffer* cmd,
                              const struct ft_buffer*         buffer,
                              uint64_t                        offset,
                              uint32_t                        draw_count,
                              uint32_t                        stride );

FT_API void
ft_create_sampler( const struct ft_device*       device,
                   const struct ft_sampler_info* info,
                   struct ft_sampler**           sampler );

FT_API void
ft_destroy_sampler( const struct ft_device* device,
                    struct ft_sampler*      sampler );

FT_API void
ft_create_image( const struct ft_device*     device,
                 const struct ft_image_info* info,
                 struct ft_image**           image );

FT_API void
ft_destroy_image( const struct ft_device* device, struct ft_image* image );

FT_API void
ft_create_descriptor_set( const struct ft_device*              device,
                          const struct ft_descriptor_set_info* info,
                          struct ft_descriptor_set**           descriptor_set );

FT_API void
ft_destroy_descriptor_set( const struct ft_device*   device,
                           struct ft_descriptor_set* set );

FT_API void
ft_update_descriptor_set( const struct ft_device*           device,
                          struct ft_descriptor_set*         set,
                          uint32_t                          count,
                          const struct ft_descriptor_write* writes );

FT_API void
ft_begin_upload_batch( void );

FT_API void
ft_end_upload_batch( void );

FT_API void
ft_upload_buffer( struct ft_buffer* buffer,
                  uint64_t          offset,
                  uint64_t          size,
                  const void*       data );

FT_API void
ft_upload_image( struct ft_image*                   image,
                 const struct ft_upload_image_info* info );

#include "renderer_misc.h"
