#version 460

layout (local_size_x = 16, local_size_y = 16) in;

const float Pi = 3.14159265359;
const int SampleCount = 64;

layout (push_constant) uniform constants
{
    uint mip_size;
    float roughness;
} PushConstants;

layout(set = 0, binding = 0) uniform samplerCube u_src_texture;
layout(set = 0, binding = 1, rgba32f) uniform imageCube u_dst_texture;

float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = Pi * denom * denom;

	return nom / denom;
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0 * Pi * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// from spherical coordinates to cartesian coordinates
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// from tangent-space vector to world-space sample vector
	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);

	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

void main()
{
	ivec3 thread_pos = ivec3(gl_GlobalInvocationID.xyz);

	float mipRoughness = PushConstants.roughness;
	uint mipSize = PushConstants.mip_size;

	if (thread_pos.x >= mipSize || thread_pos.y >= mipSize) return;

	vec2 texcoords = vec2(float(thread_pos.x + 0.5) / mipSize, float(thread_pos.y + 0.5) / mipSize);

	vec3 sphereDir = vec3(1.0);

	if (thread_pos.z <= 0)
		sphereDir = normalize(vec3(0.5, -(texcoords.y - 0.5), -(texcoords.x - 0.5)));
	else if (thread_pos.z <= 1)
		sphereDir = normalize(vec3(-0.5, -(texcoords.y - 0.5), texcoords.x - 0.5));
	else if (thread_pos.z <= 2)
		sphereDir = normalize(vec3(texcoords.x - 0.5, 0.5, texcoords.y - 0.5));
	else if (thread_pos.z <= 3)
		sphereDir = normalize(vec3(texcoords.x - 0.5, -0.5, -(texcoords.y - 0.5)));
	else if (thread_pos.z <= 4)
		sphereDir = normalize(vec3(texcoords.x - 0.5, -(texcoords.y - 0.5), 0.5));
	else if (thread_pos.z <= 5)
		sphereDir = normalize(vec3(-(texcoords.x - 0.5), -(texcoords.y - 0.5), -0.5));

	vec3 N = sphereDir;
	vec3 R = N;
	vec3 V = R;

	float totalWeight = 0.0;
	vec4 prefiltered_color = vec4(0.0, 0.0, 0.0, 0.0);
	
	vec2 dim = textureSize(u_src_texture, 1);
	float srcTextureSize = max(dim[0], dim[1]);

	for (int i = 0; i < SampleCount; ++i)
	{
		vec2 Xi = Hammersley(i, SampleCount);
		vec3 H = ImportanceSampleGGX(Xi, N, mipRoughness);
		vec3 L = normalize(2.0 * dot(V, H) * H - V);

		float NdotL = max(dot(N, L), 0.0);
		if (NdotL > 0.0)
		{
			// sample from the environment's mip level based on roughness/pdf
			float D = DistributionGGX(N, H, mipRoughness);
			float NdotH = max(dot(N, H), 0.0);
			float HdotV = max(dot(H, V), 0.0);
			float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

			float saTexel = 4.0 * Pi / (6.0 * srcTextureSize * srcTextureSize);
			float saSample = 1.0 / (float(SampleCount) * pdf + 0.0001);

			float mipLevel = mipRoughness == 0.0 ? 0.0 : max(0.5 * log2(saSample / saTexel) + 1.0f, 0.0f);

			prefiltered_color += texture(u_src_texture, L) * NdotL;

			totalWeight += NdotL;
		}
	}

	prefiltered_color = prefiltered_color / totalWeight;
    imageStore(u_dst_texture, thread_pos.xyz, prefiltered_color);
}