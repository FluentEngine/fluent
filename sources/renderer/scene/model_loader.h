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

struct Mesh
{
	u32             vertex_count;
	f32	        *positions;
	f32	        *normals;
	f32	        *tangents;
	f32	        *texcoords;
	u32             index_count;
	b32             is_32bit_indices;
	void	       *indices;
	mat4x4          world;
	struct Material material;
};

struct Model
{
	u32          mesh_count;
	struct Mesh *meshes;
};

struct Model
load_gltf( const char *filename );

void
free_model( struct Model *model );
