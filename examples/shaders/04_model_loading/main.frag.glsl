#version 450

layout (location = 0) in vec2 iTexCoord;
layout (location = 1) in vec3 iTangentLightPosition;
layout (location = 2) in vec3 iTangentViewPosition;
layout (location = 3) in vec3 iTangentFragmentPosition;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform sampler uSampler;
layout(set = 0, binding = 2) uniform texture2D uTextures[100];

layout (push_constant) uniform constants
{
    layout(offset = 96) int diffuse;
    int specular;
    int normal;
    int height;
} PushConstants;

void main()
{
    vec2 coord = iTexCoord;
    vec3 objectColor = vec3(1.0, 0.0, 1.0);
    vec3 normal = vec3(0.0, 0.0, 1.0);
    coord.y = 1.0 - coord.y;

    if (PushConstants.diffuse != -1)
    {
        objectColor = texture(sampler2D(uTextures[PushConstants.diffuse], uSampler), coord).rgb;
    }

    if (PushConstants.normal != -1)
    {
        normal = texture(sampler2D(uTextures[PushConstants.normal], uSampler), coord).rgb;
        normal = normalize(normal * 2.0 - 1.0);
    }

    vec3 ambient = 0.05 * objectColor;

    vec3 lightDir = normalize(iTangentLightPosition - iTangentFragmentPosition);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * objectColor;

    vec3 viewDir = normalize(iTangentViewPosition - iTangentFragmentPosition);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = vec3(0.3) * spec;
    FragColor = vec4(ambient + diffuse + specular, 1.0);
}