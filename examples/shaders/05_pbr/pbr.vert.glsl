#version 450

layout (location = 0) in vec3 i_position;
layout (location = 1) in vec3 i_normal;
layout (location = 2) in vec2 i_texcoord;
layout (location = 3) in vec3 i_tangent;
layout (location = 4) in vec3 i_bitangent;

layout (location = 0) out vec2 texcoord;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec3 world_position;
layout (location = 3) out vec3 camera_position;

layout (set = 0, binding = 0) uniform uCameraBuffer
{
	mat4 projection;
	mat4 view;
} ubo;

layout (push_constant) uniform constants
{
	mat4 model;
	vec4 camera_position;
} pc;

void main()
{
	normal = normalize(mat3(pc.model) * i_normal);
	texcoord = i_texcoord;
	world_position = vec3(pc.model * vec4(i_position, 1.0));
	camera_position = vec3(pc.camera_position);

    gl_Position = ubo.projection * ubo.view * vec4(world_position, 1.0);
}