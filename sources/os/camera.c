#include "input.h"
#include "camera.h"

static inline void
recalculate_projection_matrix( struct Camera* camera )
{
	mat4x4_identity( camera->projection );
	mat4x4_perspective( camera->projection,
	                    camera->fov,
	                    camera->aspect,
	                    camera->near,
	                    camera->far );
}

static inline void
recalculate_view_matrix( struct Camera* camera )
{
	vec3 center;
	vec3_add( center, camera->position, camera->direction );
	mat4x4_identity( camera->view );
	mat4x4_look_at( camera->view, camera->position, center, camera->up );
}

void
camera_init( struct Camera* camera, const struct CameraInfo* info )
{
	camera->fov    = info->fov;
	camera->aspect = info->aspect;
	camera->near   = info->near;
	camera->far    = info->far;
	camera->yaw    = -90.0f;
	camera->pitch  = 0.0f;

	vec3_dup( camera->position, info->position );
	vec3_dup( camera->direction, info->direction );
	vec3_dup( camera->world_up, info->up );
	vec3_mul_cross( camera->right, info->direction, camera->world_up );
	vec3_mul_cross( camera->up, camera->right, camera->direction );
	vec3_norm( camera->up, camera->up );

	recalculate_projection_matrix( camera );
	recalculate_view_matrix( camera );

	camera->speed             = info->speed;
	camera->mouse_sensitivity = info->sensitivity;
}

void
camera_on_move( struct Camera*       camera,
                enum CameraDirection direction,
                f32                  delta_time )
{
	f32 velocity = camera->speed * delta_time;

	switch ( direction )
	{
	case FT_CAMERA_DIRECTION_FORWARD:
	{
		vec3 p;
		vec3_dup( p, camera->direction );
		vec3_scale( p, camera->direction, velocity );
		vec3_add( camera->position, camera->position, p );
		break;
	}
	case FT_CAMERA_DIRECTION_BACK:
	{
		vec3 p;
		vec3_dup( p, camera->direction );
		vec3_scale( p, camera->direction, velocity );
		vec3_sub( camera->position, camera->position, p );
		break;
	}
	case FT_CAMERA_DIRECTION_LEFT:
	{
		vec3 p;
		vec3_dup( p, camera->right );
		vec3_scale( p, p, velocity );
		vec3_sub( camera->position, camera->position, p );
		break;
	}
	case FT_CAMERA_DIRECTION_RIGHT:
	{
		vec3 p;
		vec3_dup( p, camera->right );
		vec3_scale( p, p, velocity );
		vec3_add( camera->position, camera->position, p );
		break;
	}
	}

	recalculate_view_matrix( camera );
}

void
camera_on_rotate( struct Camera* camera, f32 x_offset, f32 y_offset )
{
	x_offset *= camera->mouse_sensitivity;
	y_offset *= camera->mouse_sensitivity;

	camera->yaw += x_offset;
	camera->pitch += y_offset;

	if ( camera->pitch > 89.0f )
		camera->pitch = 89.0f;
	if ( camera->pitch < -89.0f )
		camera->pitch = -89.0f;

	vec3 dir;
	dir[ 0 ] =
	    cosf( radians( camera->yaw ) ) * cosf( radians( camera->pitch ) );
	dir[ 1 ] = sinf( radians( camera->pitch ) );
	dir[ 2 ] =
	    sinf( radians( camera->yaw ) ) * cosf( radians( camera->pitch ) );
	vec3_norm( camera->direction, dir );

	vec3_mul_cross( camera->right, camera->direction, camera->world_up );
	vec3_norm( camera->right, camera->right );
	vec3_mul_cross( camera->up, camera->right, camera->direction );
	vec3_norm( camera->up, camera->up );

	recalculate_view_matrix( camera );
}

void
camera_on_resize( struct Camera* camera, u32 width, u32 height )
{
	camera->aspect = ( f32 ) width / ( f32 ) height;
	recalculate_projection_matrix( camera );
}

void
camera_controller_init( struct CameraController* c, struct Camera* camera )
{
	c->camera = camera;
}

void
camera_controller_update( struct CameraController* c, f32 delta_time )
{
	if ( is_key_pressed( FT_KEY_W ) )
	{
		camera_on_move( c->camera, FT_CAMERA_DIRECTION_FORWARD, delta_time );
	}
	else if ( is_key_pressed( FT_KEY_S ) )
	{
		camera_on_move( c->camera, FT_CAMERA_DIRECTION_BACK, delta_time );
	}

	if ( is_key_pressed( FT_KEY_A ) )
	{
		camera_on_move( c->camera, FT_CAMERA_DIRECTION_LEFT, delta_time );
	}
	else if ( is_key_pressed( FT_KEY_D ) )
	{
		camera_on_move( c->camera, FT_CAMERA_DIRECTION_RIGHT, delta_time );
	}

	i32 x, y;
	get_mouse_position( &x, &y );
	f32 xoffset = ( f32 ) ( x - c->last_mouse_positon[ 0 ] );
	f32 yoffset = ( f32 ) ( c->last_mouse_positon[ 1 ] - y );
	camera_on_rotate( c->camera, xoffset, yoffset );
	c->last_mouse_positon[ 0 ] = x;
	c->last_mouse_positon[ 1 ] = y;
}

void
camera_controller_reset( struct CameraController* c )
{
	get_mouse_position( &c->last_mouse_positon[ 0 ],
	                    &c->last_mouse_positon[ 1 ] );
}
