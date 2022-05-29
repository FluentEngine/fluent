#pragma once

#include "base/base.h"
#include "math/linear.h"

struct Mesh
{
	u32    vertex_count;
	f32   *positions;
	f32   *normals;
	f32   *tangents;
	f32   *texcoords;
	u32    index_count;
	b32    is_32bit_indices;
	void  *indices;
	mat4x4 world;
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
