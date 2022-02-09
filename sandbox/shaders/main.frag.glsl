#version 460

layout(location = 0) in vec3 i_frag_pos;
layout(location = 1) in vec3 i_normal;
layout(location = 2) in vec2 i_texcoord;

layout(location = 0) out vec4 out_color;

#define BASE_COLOR_IDX 0
#define NORMAL_IDX 1
#define METALLIC_ROUGHNESS_IDX 2
#define AMBIENT_OCCLUSION_IDX 3
#define EMISSIVE_TEXTURE_IDX 4
#define MATERIAL_TEXTURES_COUNT 5

layout(set = 1, binding = 0) uniform sampler u_sampler;
layout(set = 1, binding = 1) uniform texture2D u_textures[MATERIAL_TEXTURES_COUNT];

void main()
{
    vec3 base_color = texture(sampler2D(u_textures[BASE_COLOR_IDX], u_sampler), i_texcoord).rgb;
    vec3 ao = texture(sampler2D(u_textures[AMBIENT_OCCLUSION_IDX], u_sampler), i_texcoord).rgb;
    vec3 color = base_color;

    out_color = vec4(color, 1.0);
}