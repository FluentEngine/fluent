#pragma once

#include "base/base.h"
#include "math/linear.h"

struct Image;

struct ModelTexture
{
	u32   width;
	u32   height;
	void *data;
};

struct PbrMetallicRoughness
{
	struct ModelTexture base_color_texture;
};

struct Material
{
	struct PbrMetallicRoughness metallic_roughness;
};

enum AnimationInterpolation
{
	FT_ANIMATION_INTERPOLATION_STEP,
	FT_ANIMATION_INTERPOLATION_LINEAR,
	FT_ANIMATION_INTERPOLATION_SPLINE,
};

enum TransformType
{
	FT_TRANSFORM_TYPE_TRANSLATION,
	FT_TRANSFORM_TYPE_ROTATION,
	FT_TRANSFORM_TYPE_SCALE,
	FT_TRANSFORM_TYPE_WEIGHTS,
};

struct AnimationSampler
{
	u32                         time_count;
	f32                        *times;
	f32                        *values;
	enum AnimationInterpolation interpolation;
};

struct AnimationChannel
{
	struct AnimationSampler *sampler;
	enum TransformType       transform_type;
};

struct Animation
{
	f32                      duration;
	u32                      sampler_count;
	struct AnimationSampler *samplers;
	u32                      channel_count;
	struct AnimationChannel *channels;
};

struct Mesh
{
	u32             vertex_count;
	f32            *positions;
	f32            *normals;
	f32            *tangents;
	f32            *texcoords;
	f32            *joints;
	f32            *weights;
	u32             index_count;
	b32             is_32bit_indices;
	void           *indices;
	mat4x4          world;
	struct Material material;
	b32             has_rotation;
};

struct Model
{
	u32               mesh_count;
	struct Mesh      *meshes;
	u32               animation_count;
	struct Animation *animations;
};

struct Model
load_gltf( const char *filename );

void
free_gltf( struct Model *model );
