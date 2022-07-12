#pragma once

#include "base/base.h"
#include "math/linear.h"

enum ft_texture_type
{
	FT_TEXTURE_TYPE_BASE_COLOR,
	FT_TEXTURE_TYPE_NORMAL,
	FT_TEXTURE_TYPE_AMBIENT_OCCLUSION,
	FT_TEXTURE_TYPE_METAL_ROUGHNESS,
	FT_TEXTURE_TYPE_EMISSIVE,
	FT_TEXTURE_TYPE_COUNT
};

struct ft_texture
{
	uint32_t             width;
	uint32_t             height;
	uint32_t             mip_levels;
	void                *data;
	enum ft_texture_type texture_type;
};

struct ft_material
{
	int32_t textures[ FT_TEXTURE_TYPE_COUNT ];
	float4  base_color_factor;
	float3  emissive_factor;
	float   metallic_factor;
	float   roughness_factor;
	float   emissive_strength;
	int     alpha_mode;
	float   alpha_cutoff;
};

enum ft_animation_interpolation
{
	FT_ANIMATION_INTERPOLATION_STEP,
	FT_ANIMATION_INTERPOLATION_LINEAR,
	FT_ANIMATION_INTERPOLATION_SPLINE,
};

enum ft_transform_type
{
	FT_TRANSFORM_TYPE_TRANSLATION,
	FT_TRANSFORM_TYPE_ROTATION,
	FT_TRANSFORM_TYPE_SCALE,
	FT_TRANSFORM_TYPE_WEIGHTS,
};

struct ft_animation_sampler
{
	uint32_t                        frame_count;
	float                          *times;
	float                          *values;
	enum ft_animation_interpolation interpolation;
};

struct ft_animation_channel
{
	struct ft_animation_sampler *sampler;
	enum ft_transform_type       transform_type;
	uint32_t                     target;
};

struct ft_animation
{
	float                        duration;
	uint32_t                     sampler_count;
	struct ft_animation_sampler *samplers;
	uint32_t                     channel_count;
	struct ft_animation_channel *channels;
};

struct ft_mesh
{
	uint32_t           vertex_count;
	float             *positions;
	float             *normals;
	float             *tangents;
	float             *texcoords;
	float             *joints;
	float             *weights;
	uint32_t           index_count;
	uint16_t          *indices;
	float4x4           world;
	struct ft_material material;
};

struct ft_model
{
	uint32_t             mesh_count;
	struct ft_mesh      *meshes;
	uint32_t             animation_count;
	struct ft_animation *animations;
	uint32_t             texture_count;
	struct ft_texture   *textures;
};

enum ft_model_flags
{
	FT_MODEL_GENERATE_TANGENTS        = 1 << 0,
};

FT_API struct ft_model
ft_load_gltf( const char *filename, enum ft_model_flags load_flags );

FT_API void
ft_free_gltf( struct ft_model *model );
