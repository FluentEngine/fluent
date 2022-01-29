#pragma once

#include <vector>
#include "core/base.hpp"
#include "renderer/renderer.hpp"
#include "math/math.hpp"
#include "resource_manager/resource_manager.hpp"

namespace fluent
{
struct TransformComponent
{
    Matrix4 transform;
};

struct MeshComponent
{
    Geometry* geometry;
};
} // namespace fluent
