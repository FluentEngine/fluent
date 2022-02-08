#version 460

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec3 i_normal;
layout(location = 2) in vec2 i_texcoord;
layout(location = 3) in vec3 i_tangent;

layout(location = 0) out vec3 frag_pos;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec2 texcoord;

layout (set = 0, binding = 0) uniform u_camera_buffer
{
	mat4 projection;
	mat4 view;
} global_ubo;


struct ObjectData
{
	mat4 model;
};

layout(std140, set = 0, binding = 1) readonly buffer ObjectBuffer
{
	ObjectData objects[];
} object_buffer;

void main() 
{
	mat4 model_matrix = object_buffer.objects[gl_BaseInstance].model;

	frag_pos = vec3(model_matrix * vec4(i_position, 1.0));
    gl_Position = global_ubo.projection * global_ubo.view * model_matrix * vec4(i_position, 1.0);

	texcoord = i_texcoord;
}