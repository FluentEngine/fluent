#version 450

layout (location = 0) in vec2 i_texcoord;
layout (location = 1) in vec3 i_normal;
layout (location = 2) in vec3 i_world_position;
layout (location = 3) in vec3 i_camera_position;

layout(location = 0) out vec4 frag_color;

layout(set = 0, binding = 1) uniform sampler u_sampler;
layout(set = 0, binding = 2) uniform texture2D u_textures[100];
layout(set = 0, binding = 3) uniform samplerCube u_irradiance_map;
layout(set = 0, binding = 4) uniform samplerCube u_specular_map;
layout(set = 0, binding = 5) uniform texture2D u_brdf_integration_map;

layout (push_constant) uniform constants
{
    layout(offset = 64) uint albedo;
    layout(offset = 68) uint normal;
    layout(offset = 72) uint metallic_roughness;
    layout(offset = 76) uint ao;
    layout(offset = 80) uint emissive;
} pc;

const float PI = 3.14159265359;

// In future it will be a uniform
const int LIGHTS_COUNT = 1;
vec3 light_positions[LIGHTS_COUNT];
vec3 light_colors[LIGHTS_COUNT];

float distribution_ggx(vec3 N, vec3 H, float roughness);
float geometry_schlick_ggx(float N_dot_V, float roughness);
float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnel_schlick(float cos_theta, vec3 F0);
vec3 fresnel_schlick_roughness(float cos_theta, vec3 F0, float roughness);

// We have r and g channel b channel equals 1 - r ^ 2 + g ^ 2;
vec3 reconstruct_normal(vec4 sample_normal);
vec3 get_normal_from_map(vec3 normal, vec3 world_position, vec2 texcoord);

// BRDF for all imported lights
vec3 compute_brdf(vec3 N, vec3 V, vec3 L, vec3 albedo, float roughness, float metalness);
// IBL
vec3 compute_environment_brdf(vec3 N, vec3 V, vec3 albedo, float roughness, float metalness);

void main()
{
    vec2 coord = i_texcoord;
    vec3 albedo = vec3(1.0, 0.0, 1.0);
    vec3 normal = vec3(0.0, 0.0, 1.0);

    // Import
    albedo = texture(sampler2D(u_textures[pc.albedo], u_sampler), coord).rgb;
    
    albedo = pow(albedo, vec3(2.2));

    float metalness = texture(sampler2D(u_textures[pc.metallic_roughness], u_sampler), coord).b;
    float roughness = texture(sampler2D(u_textures[pc.metallic_roughness], u_sampler), coord).g;

    float ao = texture(sampler2D(u_textures[pc.ao], u_sampler), coord).r;

    // Just give values by hand. I want one red light
    light_positions[0] = vec3(1.25, 0.0, 2.0);
    light_colors[0] = vec3(1.0, 1.0, 1.0);

    vec3 N = get_normal_from_map(i_normal, i_world_position, coord);
    vec3 V = normalize(i_camera_position - i_world_position);

    // Accumulate color
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < LIGHTS_COUNT; ++i)
    {
        // I wan't spot light
        vec3 L = normalize(light_positions[i] - i_world_position);
        float distance = length(L);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = light_colors[i] * attenuation;
        
        Lo += compute_brdf(N, V, L, albedo, roughness, metalness) * radiance;
    }

    vec3 color = Lo + compute_environment_brdf(N, V, albedo, roughness, metalness) * ao;

    vec3 result_color = pow(color / (color + 1.0), vec3(1.0 / 2.2));

    if (pc.emissive != 0)
    {
        result_color += texture(sampler2D(u_textures[pc.emissive], u_sampler), coord).rgb;
    }

    frag_color = vec4(result_color, 1.0);
}

vec3 reconstruct_normal(vec4 sampleNormal)
{
	vec3 tangentNormal;
    tangentNormal.xy = (sampleNormal.rg * 2.0 - 1.0);
    tangentNormal.z = sqrt(1 - clamp(dot(tangentNormal.xy, tangentNormal.xy), 0.0, 1.0));
	return tangentNormal;
}

vec3 get_normal_from_map(vec3 Normal, vec3 WorldPos, vec2 TexCoord)
{
    vec3 tangentNormal = reconstruct_normal(texture(sampler2D(u_textures[pc.normal], u_sampler), TexCoord));
    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoord);
    vec2 st2 = dFdy(TexCoord);

    vec3 N  = normalize(Normal);
    vec3 T  = normalize(Q1 * st2.g - Q2 * st1.g);
    vec3 B  = normalize(cross(T, N));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

float distribution_ggx(vec3 N, vec3 H, float roughness)
{
    float a2    = roughness * roughness * roughness * roughness;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness)
{
    return GeometrySchlickGGX(max(dot(N, L), 0.0), roughness) * 
           GeometrySchlickGGX(max(dot(N, V), 0.0), roughness);
}

vec3 fresnel_schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnel_schlick_roughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 compute_brdf(vec3 N, vec3 V, vec3 L, vec3 albedo, float roughness, float metalness)
{
    vec3 F0 = mix(vec3(0.04), albedo, metalness);

    vec3 H = normalize(V + L);

    float NDF = distribution_ggx(N, H, roughness);
    float G = geometry_smith(N, V, L, 1.0);
    vec3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);

    float NdotL = max(dot(N, L), 0.0);

    vec3 numerator = NDF * G * F;
    float denominator = 4 * max(dot(N, V), 0.0) * NdotL;
    denominator = max(denominator, 0.0001);
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (vec3(1.0) - metalness);

    return (kD * albedo / PI + specular) * NdotL;
}

vec3 compute_environment_brdf(vec3 N, vec3 V, vec3 albedo, float roughness, float metalness)
{
    vec3 F0 = mix(vec3(0.04), albedo, metalness);
    vec3 F = fresnel_schlick_roughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metalness);

    vec3 irradiance = texture(u_irradiance_map, N).rgb;
    vec3 diffuse = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = texture(u_specular_map, reflect(-V, N), roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(sampler2D(u_brdf_integration_map, u_sampler), vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    // Ambient
    return (kD * diffuse + specular);
}