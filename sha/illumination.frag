#version 460
#extension GL_GOOGLE_include_directive : enable

layout(constant_id = 0) const int sample_count = 1;
layout(constant_id = 1) const float sample_factor = 1.0;

layout(set = 0, binding = 0) uniform Illum {
	vec3 sun;
} il;
layout(set = 0, binding = 1) uniform sampler2DMS depth_buffer;
layout(set = 0, binding = 2) uniform sampler2D depth;
layout(set = 0, binding = 3) uniform sampler2DMS albedo;
layout(set = 0, binding = 4) uniform sampler2DMS normal;

layout(location = 0) out vec3 out_output;

void main(void)
{
	out_output = vec3(0.0);
	bool sample_done[sample_count];
	for (int i = 0; i < sample_count; i++)
		sample_done[i] = false;
	int samples_done_count = 0;
	ivec2 pos = ivec2(gl_FragCoord.xy);
	while (samples_done_count < sample_count) {
		int i;
		for (i = 0; i < sample_count && sample_done[i]; i++);
		float d = texelFetch(depth_buffer, pos, i).x;
		vec3 alb = texelFetch(albedo, pos, i).xyz;
		vec3 norm = normalize(texelFetch(normal, pos, i).xyz);
		float illum = max(dot(norm, il.sun), 0.05);
		int count = 1;
		sample_done[i] = true;
		for (int j = i + 1; j < sample_count; j++) {
			bool same = texelFetch(depth_buffer, pos, j).x == d;
			sample_done[j] = sample_done[j] || same;
			count += same ? 1 : 0;
		}
		out_output += (alb * illum * 1.5) * float(count);
		samples_done_count += count;
	}
	out_output *= sample_factor;
}