#pragma once

#include <vector>
#include "core/base.hpp"
#include "renderer/renderer_backend.hpp"
#include "math/math.hpp"
#include "resource_manager/resources.hpp"

namespace fluent
{
struct TransformComponent
{
    Matrix4 transform                             = Matrix4(1.0f);
    TransformComponent()                          = default;
    TransformComponent(const TransformComponent&) = default;
    TransformComponent(const Matrix4& transform) : transform(transform)
    {
    }
};

struct MeshComponent
{
    Ref<Geometry> geometry;
    MeshComponent() = default;
};
} // namespace fluent
