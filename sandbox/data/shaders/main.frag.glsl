#version 450

layout(location = 0) in vec2 i_texcoord;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 1) uniform sampler u_sampler;
layout(set = 0, binding = 2) uniform texture2D u_terrain_map;

void main()
{
    out_color = vec4(texture(sampler2D(u_terrain_map, u_sampler), i_texcoord).rgb, 1.0);
}