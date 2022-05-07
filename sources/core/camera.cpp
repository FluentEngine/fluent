#include "core/input.hpp"
#include "core/camera.hpp"

namespace fluent
{
void
Camera::init_camera( const CameraInfo& info )
{
	fov    = info.fov;
	aspect = info.aspect;
	near   = info.near;
	far    = info.far;
	yaw    = -90.0f;
	pitch  = 0.0f;

	position  = info.position;
	direction = info.direction;
	world_up  = info.up;
	right     = glm::cross( info.direction, world_up );
	up        = normalize( glm::cross( right, direction ) );

	recalculate_projection_matrix();
	recalculate_view_matrix();

	speed             = info.speed;
	mouse_sensitivity = info.sensitivity;
}

void
Camera::recalculate_projection_matrix()
{
	projection = create_perspective_matrix( fov, aspect, near, far );
}

void
Camera::recalculate_view_matrix()
{
	view = create_look_at_matrix( position, direction, up );
}

void
Camera::on_move( CameraDirection camera_direction, f32 delta_time )
{
	f32 velocity = speed * delta_time;

	switch ( camera_direction )
	{
	case CameraDirection::eForward: position += direction * velocity; break;
	case CameraDirection::eBack: position -= direction * velocity; break;
	case CameraDirection::eLeft: position -= right * velocity; break;
	case CameraDirection::eRight: position += right * velocity; break;
	}

	recalculate_view_matrix();
}

void
Camera::on_rotate( f32 x_offset, f32 y_offset )
{
	x_offset *= mouse_sensitivity;
	y_offset *= mouse_sensitivity;

	yaw += x_offset;
	pitch += y_offset;

	if ( pitch > 89.0f )
		pitch = 89.0f;
	if ( pitch < -89.0f )
		pitch = -89.0f;

	Vector3 direction;
	direction.x     = std::cos( radians( yaw ) ) * std::cos( radians( pitch ) );
	direction.y     = std::sin( radians( pitch ) );
	direction.z     = std::sin( radians( yaw ) ) * std::cos( radians( pitch ) );
	this->direction = normalize( direction );

	right = normalize( glm::cross( this->direction, world_up ) );
	up    = normalize( glm::cross( right, this->direction ) );

	recalculate_view_matrix();
}

void
Camera::on_resize( u32 width, u32 height )
{
	aspect = ( f32 ) width / ( f32 ) height;
	recalculate_projection_matrix();
}

void
CameraController::init( Camera& camera )
{
	this->camera = &camera;
}

void
CameraController::update( f32 delta_time )
{
	if ( is_key_pressed( Key::W ) )
	{
		camera->on_move( CameraDirection::eForward, delta_time );
	}
	else if ( is_key_pressed( Key::S ) )
	{
		camera->on_move( CameraDirection::eBack, delta_time );
	}

	if ( is_key_pressed( Key::A ) )
	{
		camera->on_move( CameraDirection::eLeft, delta_time );
	}
	else if ( is_key_pressed( Key::D ) )
	{
		camera->on_move( CameraDirection::eRight, delta_time );
	}

	camera->on_rotate( get_mouse_offset_x(), -get_mouse_offset_y() );
}
} // namespace fluent
