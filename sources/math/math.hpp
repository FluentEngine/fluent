#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include "core/base.hpp"

namespace fluent
{
using Vector2 = glm::vec2;
using Vector3 = glm::vec3;
using Vector4 = glm::vec4;

using VectorInt2 = glm::ivec2;
using VectorInt3 = glm::ivec3;
using VectorInt4 = glm::ivec4;

using Matrix2 = glm::mat2;
using Matrix3 = glm::mat3;
using Matrix4 = glm::mat4;

f32 radians( f32 degrees );

Vector3 normalize( const Vector3& x );
Matrix4 create_perspective_matrix( f32 fov, f32 aspect, f32 z_near, f32 z_far );
Matrix4 create_look_at_matrix( const Vector3& position,
                               const Vector3& direction,
                               const Vector3& up );
Matrix4 rotate( const Matrix4& mat, f32 angle, Vector3 axis );
Matrix4 translate( const Matrix4& mat, const Vector3& v );

f32 perlin_noise_2d( const Vector2& v );
f32 lerp( f32 a, f32 b, f32 t );
f32 inverse_lerp( f32 a, f32 b, f32 v );

} // namespace fluent
