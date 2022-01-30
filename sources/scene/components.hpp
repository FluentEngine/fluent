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
    Matrix4 transform;
};

struct MeshComponent
{
    Ref<Geometry> geometry;
};
} // namespace fluent
