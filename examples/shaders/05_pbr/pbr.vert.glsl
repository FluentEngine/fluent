#version 450

layout (location = 0) in vec3 iPosition;
layout (location = 1) in vec3 iNormal;
layout (location = 2) in vec2 iTexCoord;
layout (location = 3) in vec3 iTangent;
layout (location = 4) in vec3 iBitangent;

layout (location = 0) out vec2 TexCoord;
layout (location = 1) out vec3 CameraPosition;
layout (location = 2) out vec3 WorldPosition;
layout (location = 3) out vec3 Normal;

layout (set = 0, binding = 0) uniform uCameraBuffer
{
	mat4 projection;
	mat4 view;
} ubo;

layout (push_constant) uniform constants
{
	mat4 model;
	vec4 lightPosition;
	vec4 viewPosition;
} PushConstants;

void main()
{
	vec3 FragmentPosition = vec3(PushConstants.model * vec4(iPosition, 1.0));

	vec3 T = normalize(vec3(PushConstants.model * vec4(iTangent, 0.0)));
	vec3 N = normalize(vec3(PushConstants.model * vec4(iNormal, 0.0)));

	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);

	mat3 TBN = mat3(T, B, N);
	TexCoord = iTexCoord;
	CameraPosition = PushConstants.viewPosition.xyz;
	WorldPosition = FragmentPosition;
	Normal = normalize(mat3(PushConstants.model) * iNormal);
	
    gl_Position = ubo.projection * ubo.view * PushConstants.model * vec4(iPosition, 1.0);
}