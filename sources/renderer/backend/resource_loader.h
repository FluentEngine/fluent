#pragma once

#include "base/base.h"
#include "renderer_enums.h"

struct ft_buffer;
struct ft_image;

struct ft_buffer_upload_job
{
	struct ft_buffer* buffer;
	uint64_t          offset;
	uint64_t          size;
	const void*       data;
};

struct ft_image_upload_job
{
	struct ft_image* image;
	const void*      data;
	uint32_t         width;
	uint32_t         height;
	uint32_t         mip_level;
};

struct ft_generate_mipmaps_job
{
	struct ft_image*       image;
	enum ft_resource_state state;
};

FT_API void
ft_resource_loader_init( const struct ft_device* device );

FT_API void
ft_resource_loader_shutdown();

FT_API void
ft_upload_buffer( const struct ft_buffer_upload_job* );

FT_API void
ft_upload_image( const struct ft_image_upload_job* );

FT_API void
ft_generate_mipmaps( const struct ft_generate_mipmaps_job* );

FT_API void
ft_resource_loader_wait_idle( void );
