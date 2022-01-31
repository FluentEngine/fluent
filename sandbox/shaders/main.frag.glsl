#version 450

layout(location = 0) in vec2 i_texcoord;

layout(location = 0) out vec4 out_color;

layout (set = 0, binding = 1) uniform sampler u_sampler;
layout (set = 1, binding = 0) uniform texture2D u_texture;

void main()
{
    vec3 color = texture(sampler2D(u_texture, u_sampler), i_texcoord).rgb;
    out_color = vec4(color, 1.0);
}