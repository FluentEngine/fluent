#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler u_sampler;
layout(binding = 2) uniform texture2D u_texture;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragUv;
layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(sampler2D(u_texture, u_sampler), fragUv);
    outColor = fragColor * texColor;
}
