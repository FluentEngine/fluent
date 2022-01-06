#version 450

layout (location = 0) in vec2 iTexCoord;
layout (location = 1) in vec3 CameraPosition;
layout (location = 2) in vec3 WorldPosition;
layout (location = 3) in vec3 Normal;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform sampler uSampler;
layout(set = 0, binding = 2) uniform texture2D uTextures[100];
layout(set = 0, binding = 3) uniform samplerCube uIrradianceMap;
layout(set = 0, binding = 4) uniform samplerCube uSpecularMap;
layout(set = 0, binding = 5) uniform texture2D uBRDFIntegrationMap;

layout (push_constant) uniform constants
{
    layout(offset = 96) int albedo;
    int normal;
    int metallic_roughness;
    int ao;
    int emissive;
} PushConstants;

const float PI = 3.14159265359;

// In future it will be a uniform
const int LIGHTS_COUNT = 1;
vec3 lightPositions[LIGHTS_COUNT];
vec3 lightColors[LIGHTS_COUNT];

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 FresnelSchlick(float cosTheta, vec3 F0);
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);

// We have r and g channel b channel equals 1 - r ^ 2 + g ^ 2;
vec3 ReconstructNormal(vec4 sampleNormal);
vec3 GetNormalFromMap(vec3 Normal, vec3 WorldPos, vec2 TexCoord);

// BRDF for all imported lights
vec3 ComputeBRDF(vec3 N, vec3 V, vec3 L, vec3 albedo, float roughness, float metalness);
// IBL
vec3 ComputeEnvironmentBRDF(vec3 N, vec3 V, vec3 albedo, float roughness, float metalness);

void main()
{
    vec2 coord = iTexCoord;
    vec3 albedo = vec3(1.0, 0.0, 1.0);
    vec3 normal = vec3(0.0, 0.0, 1.0);

    // Import
    if (PushConstants.albedo != -1)
    {
        albedo = texture(sampler2D(uTextures[PushConstants.albedo], uSampler), coord).rgb;
    }
    
    albedo = pow(albedo, vec3(2.2));
    float metalness = 0.0;
    float roughness = 0.0;

    if (PushConstants.metallic_roughness != -1)
    {
        metalness  = texture(sampler2D(uTextures[PushConstants.metallic_roughness], uSampler), coord).g;
        roughness  = texture(sampler2D(uTextures[PushConstants.metallic_roughness], uSampler), coord).b;
    }

    float ao = 0;
    if (PushConstants.ao != -1)
    {
        ao = texture(sampler2D(uTextures[PushConstants.ao], uSampler), coord).r;
    }

    // Just give values by hand. I want one red light
    lightPositions[0] = vec3(1.25, 0.0, 2.0);
    lightColors[0] = vec3(1.0, 1.0, 1.0);

    vec3 N = GetNormalFromMap(Normal, WorldPosition, coord);
    vec3 V = normalize(CameraPosition - WorldPosition);

    // Accumulate color
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < LIGHTS_COUNT; ++i)
    {
        // I wan't spot light
        vec3 L = normalize(lightPositions[i] - WorldPosition);
        float distance = length(L);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;
        
        Lo += ComputeBRDF(N, V, L, albedo, roughness, metalness) * radiance;
    }

    vec3 color = Lo + ComputeEnvironmentBRDF(N, V, albedo, roughness, metalness) * ao;
    if (PushConstants.emissive != -1)
    {
        color += texture(sampler2D(uTextures[PushConstants.emissive], uSampler), coord).rgb;
    }
    FragColor = vec4(pow(color / (color + 1.0), vec3(1.0 / 2.2)), 1.0);
}

vec3 ReconstructNormal(vec4 sampleNormal)
{
	vec3 tangentNormal;
	tangentNormal.xy = (sampleNormal.rg * 2 - 1);
	tangentNormal.z = sqrt(1 - clamp(dot(tangentNormal.xy, tangentNormal.xy), 0.0, 1.0));
	return tangentNormal;
}

vec3 GetNormalFromMap(vec3 Normal, vec3 WorldPos, vec2 TexCoord)
{
    vec3 tangentNormal = ReconstructNormal(texture(sampler2D(uTextures[PushConstants.normal], uSampler), TexCoord));
    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoord);
    vec2 st2 = dFdy(TexCoord);

    vec3 N  = normalize(Normal);
    vec3 T  = normalize(Q1 * st2.g - Q2 * st1.g);
    vec3 B  = normalize(cross(T, N));
    mat3 TBN = mat3(T, B, N);

    return normalize(tangentNormal * TBN);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
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

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    return GeometrySchlickGGX(max(dot(N, L), 0.0), roughness) * 
           GeometrySchlickGGX(max(dot(N, V), 0.0), roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 ComputeBRDF(vec3 N, vec3 V, vec3 L, vec3 albedo, float roughness, float metalness)
{
    vec3 F0 = mix(vec3(0.04), albedo, metalness);

    vec3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, 1.0);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float NdotL = max(dot(N, L), 0.0);

    vec3 numerator = NDF * G * F;
    float denominator = 4 * max(dot(N, V), 0.0) * NdotL;
    denominator = max(denominator, 0.0001);
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (vec3(1.0) - metalness);

    return (kD * albedo / PI + specular) * NdotL;
}

vec3 ComputeEnvironmentBRDF(vec3 N, vec3 V, vec3 albedo, float roughness, float metalness)
{
    vec3 F0 = mix(vec3(0.04), albedo, metalness);
    vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metalness);

    vec3 irradiance = texture(uIrradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = texture(uSpecularMap, reflect(-V, N), roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(sampler2D(uBRDFIntegrationMap, uSampler), vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    // Ambient
    return (kD * diffuse + specular);
}