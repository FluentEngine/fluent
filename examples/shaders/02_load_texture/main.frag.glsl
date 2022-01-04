#version 450

layout(location = 0) in vec3 i_color;
layout(location = 1) in vec2 i_tex_coord;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler u_sampler;
layout(set = 0, binding = 1) uniform texture2D u_texture;

void main()
{
    out_color = texture(sampler2D(u_texture, u_sampler), i_tex_coord);
}