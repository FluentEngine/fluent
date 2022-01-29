#pragma once

#include "core/base.hpp"
#include "math/math.hpp"

namespace fluent
{
enum class CameraDirection
{
    eForward,
    eBack,
    eLeft,
    eRight
};

struct CameraData
{
    Matrix4 projection;
    Matrix4 view;
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

    Vector3    m_position;
    Vector3    m_direction;
    Vector3    m_up;
    Vector3    m_world_up;
    Vector3    m_right;
    CameraData m_data;

    f32 m_speed;
    f32 m_mouse_sensitivity;

    void recalculate_projection_matrix();
    void recalculate_view_matrix();

public:
    void init_camera(fluent::Vector3 position, fluent::Vector3 direction, fluent::Vector3 up);

    void on_move(CameraDirection direction, f32 delta_time);
    void on_rotate(f32 x_offset, f32 y_offset);

    void on_resize(u32 width, u32 height);

    const CameraData& get_data() const
    {
        return m_data;
    }

    const Vector3& get_position() const
    {
        return m_position;
    }
};

class CameraController
{
private:
    const InputSystem* m_input_system = nullptr;
    Camera*            m_camera       = nullptr;

public:
    void init(const InputSystem* input_system, Camera& camera);
    void update(f32 delta_time);
};
}