#pragma once

#include "fluent/fluent.hpp"

enum class CameraDirection
{
    eForward,
    eBack,
    eLeft,
    eRight
};

struct CameraData
{
    fluent::Matrix4 projection;
    fluent::Matrix4 view;
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

    fluent::Vector3 m_position;
    fluent::Vector3 m_direction;
    fluent::Vector3 m_up;
    fluent::Vector3 m_world_up;
    fluent::Vector3 m_right;
    CameraData      m_data;

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
};

class CameraController
{
private:
    const fluent::InputSystem* m_input_system = nullptr;
    Camera*                    m_camera       = nullptr;

public:
    void init(const fluent::InputSystem* input_system, Camera& camera);
    void update(f32 delta_time);
};