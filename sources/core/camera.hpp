#pragma once

#include "core/base.hpp"
#include "core/input.hpp"
#include "math/math.hpp"
#undef near
#undef far

namespace fluent
{
enum class CameraDirection
{
	eForward,
	eBack,
	eLeft,
	eRight
};

struct CameraInfo
{
	fluent::Vector3 position;
	fluent::Vector3 direction;
	fluent::Vector3 up;
	f32             aspect      = 1400.0f / 900.0f;
	f32             fov         = radians( 45.0f );
	f32             near        = 0.1f;
	f32             far         = 1000.0f;
	f32             speed       = 5.0f;
	f32             sensitivity = 0.1f;
};

class Camera
{
private:
	f32 fov;
	f32 aspect;
	f32 near;
	f32 far;
	f32 yaw;
	f32 pitch;

	Vector3 position;
	Vector3 direction;
	Vector3 up;
	Vector3 world_up;
	Vector3 right;
	Matrix4 projection;
	Matrix4 view;

	f32 speed;
	f32 mouse_sensitivity;

	void
	recalculate_projection_matrix();
	void
	recalculate_view_matrix();

public:
	void
	init_camera( const CameraInfo& info );

	void
	on_move( CameraDirection direction, f32 delta_time );
	void
	on_rotate( f32 x_offset, f32 y_offset );

	void
	on_resize( u32 width, u32 height );

	const Matrix4&
	get_projection_matrix() const
	{
		return projection;
	}

	const Matrix4&
	get_view_matrix() const
	{
		return view;
	}

	const Vector3&
	get_position() const
	{
		return position;
	}

	const Vector3&
	get_direction() const
	{
		return direction;
	}
};

class CameraController
{
private:
	Camera* camera = nullptr;

public:
	void
	init( Camera& camera );
	void
	update( f32 delta_time );
};
} // namespace fluent
