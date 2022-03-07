#include "core/input.hpp"
#include "core/camera.hpp"

namespace fluent
{
void Camera::init_camera( const CameraDesc& desc )
{
    m_fov    = desc.fov;
    m_aspect = desc.aspect;
    m_near   = desc.near;
    m_far    = desc.far;
    m_yaw    = -90.0f;
    m_pitch  = 0.0f;

    m_position  = desc.position;
    m_direction = desc.direction;
    m_world_up  = desc.up;
    m_right     = glm::cross( desc.direction, m_world_up );
    m_up        = normalize( glm::cross( m_right, m_direction ) );

    recalculate_projection_matrix();
    recalculate_view_matrix();

    m_speed             = desc.speed;
    m_mouse_sensitivity = desc.sensitivity;
}

void Camera::recalculate_projection_matrix()
{
    m_projection = create_perspective_matrix( m_fov, m_aspect, m_near, m_far );
}

void Camera::recalculate_view_matrix()
{
    m_view = create_look_at_matrix( m_position, m_direction, m_up );
}

void Camera::on_move( CameraDirection direction, f32 delta_time )
{
    f32 velocity = m_speed * delta_time;

    switch ( direction )
    {
    case CameraDirection::eForward: m_position += m_direction * velocity; break;
    case CameraDirection::eBack: m_position -= m_direction * velocity; break;
    case CameraDirection::eLeft: m_position -= m_right * velocity; break;
    case CameraDirection::eRight: m_position += m_right * velocity; break;
    }

    recalculate_view_matrix();
}

void Camera::on_rotate( f32 x_offset, f32 y_offset )
{
    x_offset *= m_mouse_sensitivity;
    y_offset *= m_mouse_sensitivity;

    m_yaw += x_offset;
    m_pitch += y_offset;

    if ( m_pitch > 89.0f )
        m_pitch = 89.0f;
    if ( m_pitch < -89.0f )
        m_pitch = -89.0f;

    Vector3 direction;
    direction.x = cos( radians( m_yaw ) ) * cos( radians( m_pitch ) );
    direction.y = sin( radians( m_pitch ) );
    direction.z = sin( radians( m_yaw ) ) * cos( radians( m_pitch ) );
    m_direction = normalize( direction );

    m_right = normalize( glm::cross( m_direction, m_world_up ) );
    m_up    = normalize( glm::cross( m_right, m_direction ) );

    recalculate_view_matrix();
}

void Camera::on_resize( u32 width, u32 height )
{
    m_aspect = ( f32 ) width / ( f32 ) height;
    recalculate_projection_matrix();
}

void CameraController::init( const fluent::InputSystem* input_system,
                             Camera&                    camera )
{
    m_input_system = input_system;
    m_camera       = &camera;
}

void CameraController::update( f32 delta_time )
{
    if ( is_key_pressed( m_input_system, Key::W ) )
    {
        m_camera->on_move( CameraDirection::eForward, delta_time );
    }
    else if ( is_key_pressed( m_input_system, Key::S ) )
    {
        m_camera->on_move( CameraDirection::eBack, delta_time );
    }

    if ( is_key_pressed( m_input_system, Key::A ) )
    {
        m_camera->on_move( CameraDirection::eLeft, delta_time );
    }
    else if ( is_key_pressed( m_input_system, Key::D ) )
    {
        m_camera->on_move( CameraDirection::eRight, delta_time );
    }

    m_camera->on_rotate( get_mouse_offset_x( m_input_system ),
                         -get_mouse_offset_y( m_input_system ) );
}
} // namespace fluent
