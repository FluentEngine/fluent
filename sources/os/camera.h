#pragma once

#include "base/base.h"
#include "math/linear.h"

enum CameraDirection
{
	FT_CAMERA_DIRECTION_FORWARD,
	FT_CAMERA_DIRECTION_BACK,
	FT_CAMERA_DIRECTION_LEFT,
	FT_CAMERA_DIRECTION_RIGHT
};

struct CameraInfo
{
	vec3 position;
	vec3 direction;
	vec3 up;
	f32  aspect;
	f32  fov;
	f32  near;
	f32  far;
	f32  speed;
	f32  sensitivity;
};

struct Camera
{
	f32 fov;
	f32 aspect;
	f32 near;
	f32 far;
	f32 yaw;
	f32 pitch;

	vec3 position;
	vec3 direction;
	vec3 up;
	vec3 world_up;
	vec3 right;
	mat4x4 projection;
	mat4x4 view;

	f32 speed;
	f32 mouse_sensitivity;
};

void
camera_init( struct Camera*, const struct CameraInfo* info );

void
camera_on_move( struct Camera*, enum CameraDirection direction, f32 delta_time );

void
camera_on_rotate( struct Camera*, f32 x_offset, f32 y_offset );

void
camera_on_resize( struct Camera*, u32 width, u32 height );

struct CameraController
{
	struct Camera* camera;
};

void
camera_controller_init( struct CameraController*, struct Camera* );

void
camera_controller_update( struct CameraController*, f32 delta_time );
