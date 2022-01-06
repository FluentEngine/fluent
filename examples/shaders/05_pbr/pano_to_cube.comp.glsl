#version 460

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout (push_constant) uniform constants
{
    uint mip;
    uint max_size;
} PushConstants;

layout(set = 0, binding = 0) uniform texture2D u_equirectangular_map;
layout(set = 0, binding = 1) uniform sampler u_skybox_sampler;

layout(set = 0, binding = 2, rgba32f) uniform imageCube u_output_texture;

void main()
{
    vec2 inv_atan = vec2(0.1591, 0.3183);
	vec3 thread_pos = vec3(gl_GlobalInvocationID.xyz);
    uint mip_level = PushConstants.mip;

    uint mip_size = PushConstants.max_size >> mip_level;
    if (thread_pos.x >= mip_size || thread_pos.y >= mip_size) return;

    vec2 texcoords = vec2(float(thread_pos.x + 0.5) / mip_size, 1.0 - float(thread_pos.y + 0.5) / mip_size);
    vec3 sphere_dir = vec3(1.0);
    if (thread_pos.z <= 0)
        sphere_dir = normalize(vec3(0.5, -(texcoords.y - 0.5), -(texcoords.x - 0.5)));
    else if (thread_pos.z <= 1)
        sphere_dir = normalize(vec3(-0.5, -(texcoords.y - 0.5), texcoords.x - 0.5));
    else if (thread_pos.z <= 2)
        sphere_dir = normalize(vec3(texcoords.x - 0.5, -0.5, -(texcoords.y - 0.5)));
    else if (thread_pos.z <= 3)
        sphere_dir = normalize(vec3(texcoords.x - 0.5, 0.5, texcoords.y - 0.5));
    else if (thread_pos.z <= 4)
        sphere_dir = normalize(vec3(texcoords.x - 0.5, -(texcoords.y - 0.5), 0.5));
    else if (thread_pos.z <= 5)
        sphere_dir = normalize(vec3(-(texcoords.x - 0.5), -(texcoords.y - 0.5), -0.5));

    vec2 panoUVs = vec2(atan(sphere_dir.z, sphere_dir.x), asin(sphere_dir.y));
    panoUVs *= inv_atan;
    panoUVs += 0.5f;

    imageStore(u_output_texture, ivec3(thread_pos.xyz), texture(sampler2D(u_equirectangular_map, u_skybox_sampler), panoUVs));
}