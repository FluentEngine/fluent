#version 450

layout(location = 0) in vec2 i_texcoord;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 1) uniform sampler u_sampler;

void main()
{
    out_color = vec4(vec3(0.0), 1.0);
}