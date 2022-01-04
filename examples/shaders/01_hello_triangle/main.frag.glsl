#version 450

layout(location = 0) in vec3 iColor;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(iColor, 1.0);
}