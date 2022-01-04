#version 450

layout(location = 0) in vec2 i_position; 
layout(location = 1) in vec2 i_tex_coord;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 tex_coord;

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = vec4(i_position, 0.0, 1.0);
    frag_color = colors[gl_VertexIndex];
    tex_coord = i_tex_coord;
}