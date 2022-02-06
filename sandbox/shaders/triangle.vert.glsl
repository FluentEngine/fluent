#version 450

layout(location = 0) in vec3 i_position; 

layout (set = 0, binding = 0) uniform u_camera_buffer
{
	mat4 projection;
	mat4 view;
} global_ubo;

void main() 
{
    gl_Position = global_ubo.projection * global_ubo.view * vec4(i_position, 1.0);
}