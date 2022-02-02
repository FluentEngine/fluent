#version 450

layout(location = 0) in vec3 i_frag_pos;
layout(location = 1) in vec3 i_normal;
layout(location = 2) in vec2 i_texcoord;
layout(location = 3) in vec3 i_viewer_position;

layout(location = 0) out vec4 out_color;

layout (set = 0, binding = 1) uniform sampler u_sampler;
layout (set = 1, binding = 0) uniform texture2D u_texture;

struct Light
{
    vec3 position;
    vec3 color;
};

void main()
{
    Light light;
    light.position = vec3(1.0, 10.0, -1.0);
    light.color = vec3(1.0);
    
    float ambient_strength = 0.1;
    vec3 ambient = ambient_strength * light.color;
    
    vec3 N = normalize(i_normal);
    vec3 light_dir = normalize(light.position - i_frag_pos);

    float diff = max(dot(N, light_dir), 0.0);
    vec3 diffuse = diff * light.color;

    float specular_strength = 0.5;

    vec3 view_dir = normalize(i_viewer_position - i_frag_pos);
    vec3 reflect_dir = reflect(-light_dir, N);  

    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    vec3 specular = specular_strength * spec * light.color;  

    vec3 object_color = texture(sampler2D(u_texture, u_sampler), i_texcoord).rgb;
    vec3 result = (ambient + diffuse) * object_color;

    out_color = vec4(result, 1.0);
}