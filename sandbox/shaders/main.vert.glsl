#version 450

layout(location = 0) in vec3 i_position; 
layout(location = 1) in vec3 i_normal; 
layout(location = 2) in vec2 i_texcoord; 
layout(location = 3) in vec3 i_tangent; 
layout(location = 4) in vec3 i_bitangent; 

layout(location = 0) out vec3 frag_pos;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec2 tex_coord;
layout(location = 3) out vec3 viewer_position;

layout (set = 0, binding = 0) uniform u_camera_buffer
{
	mat4 projection;
	mat4 view;
} ubo;

layout (push_constant) uniform constants
{
	mat4 model;
	vec4 viewer_position;
} pc;

void main() 
{
    gl_Position = ubo.projection * ubo.view * pc.model * vec4(i_position, 1.0);
	frag_pos = vec3(pc.model * vec4(i_position, 1.0));
	normal = transpose(inverse(mat3(pc.model))) * i_normal;
	tex_coord = i_texcoord;
	viewer_position = vec3(pc.viewer_position);
}