#version 450

layout(location = 0) in vec3 i_position; 
layout(location = 1) in vec3 i_normal; 
layout(location = 2) in vec2 i_texcoord; 
layout(location = 3) in vec3 i_tangent; 
layout(location = 4) in vec3 i_bitangent; 

layout (set = 0, binding = 0) uniform u_camera_buffer
{
	mat4 projection;
	mat4 view;
} ubo;

layout (push_constant) uniform constants
{
	mat4 model;
} pc;

void main() 
{
    gl_Position = ubo.projection * ubo.view * pc.model * vec4(i_position, 1.0);
}