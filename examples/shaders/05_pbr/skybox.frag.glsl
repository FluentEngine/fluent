#version 450 core

layout(location = 0) in vec3 WorldPos;

layout(set = 0, binding = 1) uniform samplerCube u_skybox;

layout(location = 0) out vec4 FragColor;

void main()
{		
    FragColor = texture(u_skybox, WorldPos);
}