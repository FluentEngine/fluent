#include "input.h"
#include "camera.h"

FT_INLINE void
recalculate_projection_matrix( struct ft_camera* camera )
{
	float4x4_identity( camera->projection );
	float4x4_perspective( camera->projection,
	                      camera->fov,
	                      camera->aspect,
	                      camera->near,
	                      camera->far );
}

FT_INLINE void
recalculate_view_matrix( struct ft_camera* camera )
{
	float3 center;
	float3_add( center, camera->position, camera->direction );
	float4x4_identity( camera->view );
	float4x4_look_at( camera->view, camera->position, center, camera->up );
}

void
ft_camera_init( struct ft_camera* camera, const struct ft_camera_info* info )
{
	camera->fov    = info->fov;
	camera->aspect = info->aspect;
	camera->near   = info->near;
	camera->far    = info->far;
	camera->yaw    = -90.0f;
	camera->pitch  = 0.0f;

	float3_dup( camera->position, info->position );
	float3_dup( camera->direction, info->direction );
	float3_dup( camera->world_up, info->up );
	float3_mul_cross( camera->right, info->direction, camera->world_up );
	float3_mul_cross( camera->up, camera->right, camera->direction );
	float3_norm( camera->up, camera->up );

	recalculate_projection_matrix( camera );
	recalculate_view_matrix( camera );

	camera->speed             = info->speed;
	camera->mouse_sensitivity = info->sensitivity;
}

void
ft_camera_on_move( struct ft_camera*        camera,
                   enum ft_camera_direction direction,
                   float                    delta_time )
{
	float velocity = camera->speed * delta_time;

	switch ( direction )
	{
	case FT_CAMERA_DIRECTION_FORWARD:
	{
		float3 p;
		float3_dup( p, camera->direction );
		float3_scale( p, camera->direction, velocity );
		float3_add( camera->position, camera->position, p );
		break;
	}
	case FT_CAMERA_DIRECTION_BACK:
	{
		float3 p;
		float3_dup( p, camera->direction );
		float3_scale( p, camera->direction, velocity );
		float3_sub( camera->position, camera->position, p );
		break;
	}
	case FT_CAMERA_DIRECTION_LEFT:
	{
		float3 p;
		float3_dup( p, camera->right );
		float3_scale( p, p, velocity );
		float3_sub( camera->position, camera->position, p );
		break;
	}
	case FT_CAMERA_DIRECTION_RIGHT:
	{
		float3 p;
		float3_dup( p, camera->right );
		float3_scale( p, p, velocity );
		float3_add( camera->position, camera->position, p );
		break;
	}
	}

	recalculate_view_matrix( camera );
}

void
ft_camera_on_rotate( struct ft_camera* camera, float x_offset, float y_offset )
{
	x_offset *= camera->mouse_sensitivity;
	y_offset *= camera->mouse_sensitivity;

	camera->yaw += x_offset;
	camera->pitch += y_offset;

	if ( camera->pitch > 89.0f )
		camera->pitch = 89.0f;
	if ( camera->pitch < -89.0f )
		camera->pitch = -89.0f;

	float3 dir;
	dir[ 0 ] =
	    cosf( radians( camera->yaw ) ) * cosf( radians( camera->pitch ) );
	dir[ 1 ] = sinf( radians( camera->pitch ) );
	dir[ 2 ] =
	    sinf( radians( camera->yaw ) ) * cosf( radians( camera->pitch ) );
	float3_norm( camera->direction, dir );

	float3_mul_cross( camera->right, camera->direction, camera->world_up );
	float3_norm( camera->right, camera->right );
	float3_mul_cross( camera->up, camera->right, camera->direction );
	float3_norm( camera->up, camera->up );

	recalculate_view_matrix( camera );
}

void
ft_camera_on_resize( struct ft_camera* camera, uint32_t width, uint32_t height )
{
	camera->aspect = ( float ) width / ( float ) height;
	recalculate_projection_matrix( camera );
}

void
ft_camera_controller_init( struct ft_camera_controller* c,
                           struct ft_camera*            camera )
{
	c->camera = camera;
}

void
ft_camera_controller_update( struct ft_camera_controller* c, float delta_time )
{
	if ( ft_is_key_pressed( FT_KEY_W ) )
	{
		ft_camera_on_move( c->camera, FT_CAMERA_DIRECTION_FORWARD, delta_time );
	}
	else if ( ft_is_key_pressed( FT_KEY_S ) )
	{
		ft_camera_on_move( c->camera, FT_CAMERA_DIRECTION_BACK, delta_time );
	}

	if ( ft_is_key_pressed( FT_KEY_A ) )
	{
		ft_camera_on_move( c->camera, FT_CAMERA_DIRECTION_LEFT, delta_time );
	}
	else if ( ft_is_key_pressed( FT_KEY_D ) )
	{
		ft_camera_on_move( c->camera, FT_CAMERA_DIRECTION_RIGHT, delta_time );
	}

	int32_t x, y;
	ft_get_mouse_position( &x, &y );
	float xoffset = ( float ) ( x - c->last_mouse_positon[ 0 ] );
	float yoffset = ( float ) ( c->last_mouse_positon[ 1 ] - y );
	ft_camera_on_rotate( c->camera, xoffset, yoffset );
	c->last_mouse_positon[ 0 ] = x;
	c->last_mouse_positon[ 1 ] = y;
}

void
ft_camera_controller_reset( struct ft_camera_controller* c )
{
	ft_get_mouse_position( &c->last_mouse_positon[ 0 ],
	                       &c->last_mouse_positon[ 1 ] );
}
