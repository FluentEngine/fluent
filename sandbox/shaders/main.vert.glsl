#version 460

layout(location = 0) in vec3 i_position; 
layout(location = 0) out vec3 color;

layout (set = 0, binding = 0) uniform u_camera_buffer
{
	mat4 projection;
	mat4 view;
} global_ubo;


struct ObjectData
{
	mat4 model;
	vec4 color;
};

layout(std140, set = 0, binding = 1) readonly buffer ObjectBuffer
{
	ObjectData objects[];
} object_buffer;

void main() 
{
	color = object_buffer.objects[gl_BaseInstance].color.rgb;
	mat4 model_matrix = object_buffer.objects[gl_BaseInstance].model;

    gl_Position = global_ubo.projection * global_ubo.view * model_matrix * vec4(i_position, 1.0);
}