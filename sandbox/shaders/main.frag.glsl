#version 460

layout(location = 0) in vec3 i_frag_pos;
layout(location = 1) in vec3 i_normal;
layout(location = 2) in vec2 i_texcoord;

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform sampler u_sampler;
layout(set = 1, binding = 1) uniform texture2D u_base_color_texture;
layout(set = 1, binding = 2) uniform texture2D u_normal_texture;
layout(set = 1, binding = 3) uniform texture2D u_metallic_roughness_texture;
layout(set = 1, binding = 4) uniform texture2D u_occlusion_texture;
layout(set = 1, binding = 5) uniform texture2D u_emissive_texture;

void main()
{
    vec3 color = texture(sampler2D(u_base_color_texture, u_sampler), i_texcoord).rgb;

    out_color = vec4(color, 1.0);
}