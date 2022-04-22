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

struct CameraDesc
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
    f32 m_fov;
    f32 m_aspect;
    f32 m_near;
    f32 m_far;
    f32 m_yaw;
    f32 m_pitch;

    Vector3 m_position;
    Vector3 m_direction;
    Vector3 m_up;
    Vector3 m_world_up;
    Vector3 m_right;
    Matrix4 m_projection;
    Matrix4 m_view;

    f32 m_speed;
    f32 m_mouse_sensitivity;

    void
    recalculate_projection_matrix();
    void
    recalculate_view_matrix();

public:
    void
    init_camera( const CameraDesc& desc );

    void
    on_move( CameraDirection direction, f32 delta_time );
    void
    on_rotate( f32 x_offset, f32 y_offset );

    void
    on_resize( u32 width, u32 height );

    const Matrix4&
    get_projection_matrix() const
    {
        return m_projection;
    }

    const Matrix4&
    get_view_matrix() const
    {
        return m_view;
    }

    const Vector3&
    get_position() const
    {
        return m_position;
    }

    const Vector3&
    get_direction() const
    {
        return m_direction;
    }
};

class CameraController
{
private:
	Camera* m_camera = nullptr;

public:
    void
	init( Camera& camera );
    void
    update( f32 delta_time );
};
} // namespace fluent
