#pragma once

#include "base/base.h"
#include "math/linear.h"

enum ft_camera_direction
{
	FT_CAMERA_DIRECTION_FORWARD,
	FT_CAMERA_DIRECTION_BACK,
	FT_CAMERA_DIRECTION_LEFT,
	FT_CAMERA_DIRECTION_RIGHT
};

struct ft_camera_info
{
	float3 position;
	float3 direction;
	float3 up;
	float  aspect;
	float  fov;
	float  near;
	float  far;
	float  speed;
	float  sensitivity;
};

struct ft_camera
{
	float fov;
	float aspect;
	float near;
	float far;
	float yaw;
	float pitch;

	float3   position;
	float3   direction;
	float3   up;
	float3   world_up;
	float3   right;
	float4x4 projection;
	float4x4 view;

	float speed;
	float mouse_sensitivity;
};

FT_API void
ft_camera_init( struct ft_camera*, const struct ft_camera_info* info );

FT_API void
ft_camera_on_move( struct ft_camera*,
                   enum ft_camera_direction direction,
                   float                    delta_time );

FT_API void
ft_camera_on_rotate( struct ft_camera*, float x_offset, float y_offset );

FT_API void
ft_camera_on_resize( struct ft_camera*, uint32_t width, uint32_t height );

struct ft_camera_controller
{
	struct ft_camera* camera;
	int32_t           last_mouse_positon[ 2 ];
};

FT_API void
ft_camera_controller_init( struct ft_camera_controller*, struct ft_camera* );

FT_API void
ft_camera_controller_update( struct ft_camera_controller*, float delta_time );

FT_API void
ft_camera_controller_reset( struct ft_camera_controller* );
