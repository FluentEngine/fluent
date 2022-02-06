#version 460

layout(location = 0) in vec3 i_color;
layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform sampler u_sampler;
layout(set = 1, binding = 1) uniform texture2D u_base_color_texture;
layout(set = 1, binding = 2) uniform texture2D u_normal_texture;
layout(set = 1, binding = 3) uniform texture2D u_metallic_roughness_texture;
layout(set = 1, binding = 4) uniform texture2D u_occlusion_texture;
layout(set = 1, binding = 5) uniform texture2D u_emissive_texture;

void main()
{
    out_color = vec4(i_color, 1.0);
}