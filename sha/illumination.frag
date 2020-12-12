#version 460
#extension GL_GOOGLE_include_directive : enable

layout(constant_id = 0) const int sample_count = 1;
layout(constant_id = 1) const float sample_factor = 1.0;

layout(set = 0, binding = 0) uniform Illum {
	vec3 sun;
} il;
layout(set = 0, binding = 1) uniform sampler2DMS cdepth;
layout(set = 0, binding = 2) uniform sampler2D depth;
layout(set = 0, binding = 3) uniform sampler2DMS albedo;
layout(set = 0, binding = 4) uniform sampler2DMS normal;

layout(location = 0) out vec3 out_output;

void main(void)
{
	ivec2 pos = ivec2(gl_FragCoord.xy);
	out_output = vec3(0.0);
	int sample_done[sample_count];
	//int its = 0;
	for (int i = 0; i < sample_count; i++)
		sample_done[i] = 0;
	int i = 0;
	for (int k = 0; k < sample_count; k++) {
		float d = texelFetch(cdepth, pos, i).x;
		vec3 alb = texelFetch(albedo, pos, i).xyz;
		vec3 norm = normalize(texelFetch(normal, pos, i).xyz);
		float illum = max(dot(norm, il.sun), 0.05);
		vec3 outp = alb * illum * 1.5;

		float count = 0;
		for (int j = 0; j < sample_count; j++) {
			bool same = texelFetch(cdepth, pos, j).x == d;
			sample_done[j] += same ? 1 : 0;
			count += same ? 1.0 : 0.0;
		}
		out_output += outp * count;
		//its++;

		for (int j = 0; j < sample_count; j++) {
			if (sample_done[i] == 0)
				break;
			i++;
			if (i >= sample_count) {
				out_output *= sample_factor;
				//out_output = vec3(float(its) / float(sample_count));
				return;
			}
		}
	}
}