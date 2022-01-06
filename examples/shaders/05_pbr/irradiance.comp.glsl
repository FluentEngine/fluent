#version 460

layout (local_size_x = 16, local_size_y = 16) in;

const float Pi = 3.14159265359;
float SampleDelta = 0.025;

layout(set = 0, binding = 0) uniform samplerCube u_src_texture;
layout(set = 0, binding = 1, rgba32f) uniform imageCube u_dst_texture;

vec4 computeIrradiance(vec3 N)
{
	vec4 irradiance = vec4(0.0);

	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = cross(up, N);
	up = cross(N, right);

	float nrSamples = 0.0;

	for (float phi = 0.0; phi < 2.0 * Pi; phi += SampleDelta)
	{
		for (float theta = 0.0; theta < 0.5 * Pi; theta += SampleDelta)
		{
			// spherical to cartesian (in tangent space)
			vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

			// tangent space to world
			vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

			vec4 sampledValue = texture(u_src_texture, sampleVec);
		
			irradiance += vec4(sampledValue.rgb * cos(theta) * sin(theta), sampledValue.a);
			nrSamples++;

		}
	}

	return Pi * irradiance * (1.0 / float(nrSamples));
}

void main()
{
	vec3 thread_pos = vec3(gl_GlobalInvocationID.xyz);
	uint pixel_offset = 0;

	for (uint i = 0; i < thread_pos.z; ++i)
	{
		pixel_offset += 32 * 32;
	}

	vec2 texcoords = vec2(float(thread_pos.x + 0.5) / 32.0f, float(thread_pos.y + 0.5) / 32.0f);

	vec3 sphere_dir = vec3(1.0);

	if (thread_pos.z <= 0)
		sphere_dir = normalize(vec3(0.5, -(texcoords.y - 0.5), -(texcoords.x - 0.5)));
	else if (thread_pos.z <= 1)
		sphere_dir = normalize(vec3(-0.5, -(texcoords.y - 0.5), texcoords.x - 0.5));
	else if (thread_pos.z <= 2)
		sphere_dir = normalize(vec3(texcoords.x - 0.5, 0.5, texcoords.y - 0.5));
	else if (thread_pos.z <= 3)
		sphere_dir = normalize(vec3(texcoords.x - 0.5, -0.5, -(texcoords.y - 0.5)));
	else if (thread_pos.z <= 4)
		sphere_dir = normalize(vec3(texcoords.x - 0.5, -(texcoords.y - 0.5), 0.5));
	else if (thread_pos.z <= 5)
		sphere_dir = normalize(vec3(-(texcoords.x - 0.5), -(texcoords.y - 0.5), -0.5));

	vec4 irradiance = computeIrradiance(sphere_dir);

    imageStore(u_dst_texture, ivec3(thread_pos.xyz), irradiance);
}
