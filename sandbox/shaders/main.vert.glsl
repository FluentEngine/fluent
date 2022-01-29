#version 450

layout(location = 0) in vec3 i_position; 

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