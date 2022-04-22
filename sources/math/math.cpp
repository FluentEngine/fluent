#include "math/math.hpp"

namespace fluent
{
f32
radians( f32 degrees )
{
    return glm::radians( degrees );
}

Matrix4
create_perspective_matrix( f32 fov, f32 aspect, f32 z_near, f32 z_far )
{
    return glm::perspective( fov, aspect, z_near, z_far );
}

Matrix4
create_look_at_matrix( const Vector3& position,
                       const Vector3& direction,
                       const Vector3& up )
{
    return glm::lookAt( position, position + direction, up );
}

Matrix4
rotate( const Matrix4& mat, f32 angle, Vector3 axis )
{
    return glm::rotate( mat, angle, axis );
}

Matrix4
translate( const Matrix4& mat, const Vector3& v )
{
    return glm::translate( mat, v );
}

Vector3
normalize( const Vector3& x )
{
    return glm::normalize( x );
}

f32
perlin_noise_2d( const Vector2& v )
{
    return glm::perlin( v );
}

f32
lerp( f32 a, f32 b, f32 t )
{
    return glm::mix( a, b, t );
}

f32
inverse_lerp( f32 a, f32 b, f32 v )
{
    return ( v - a ) / ( b - a );
}

} // namespace fluent
