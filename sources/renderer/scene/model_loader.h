#pragma once

#include "base/base.h"
#include "math/linear.h"

struct ft_texture
{
	uint32_t width;
	uint32_t height;
	void    *data;
};

struct ft_pbr_metallic_roughness
{
	uint32_t                base_color_texture;
	float4                  base_color_factor;
	float                   metallic_factor;
	float                   roughness_factor;
};

struct ft_material
{
	struct ft_pbr_metallic_roughness metallic_roughness;
	uint32_t                         normal_texture;
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
	bool               is_32bit_indices;
	void              *indices;
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

FT_API struct ft_model
ft_load_gltf( const char *filename );

FT_API void
ft_free_gltf( struct ft_model *model );
