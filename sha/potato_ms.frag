#version 460
#extension GL_GOOGLE_include_directive : enable

layout(constant_id = 0) const int sample_count = 1;
layout(constant_id = 1) const float sample_factor = 1.0;

#include "illum.glsl"

layout(set = 0, binding = 1) uniform sampler2DMS cdepth;
layout(set = 0, binding = 2) uniform sampler2DMS albedo;
layout(set = 0, binding = 3) uniform sampler2DMS normal;

layout(location = 0) out vec3 out_output;

#include "rt.glsl"

vec3 rt_pos_view(vec2 pos, int samp)
{
	return rt_pos_view(pos, texelFetch(cdepth, ivec2(pos), samp).x);
}

vec3 rt_current_view(int samp)
{
	return rt_pos_view(gl_FragCoord.xy, samp);
}

void main(void)
{
	ivec2 pos = ivec2(gl_FragCoord.xy);

	int sample_done[sample_count];
	for (int i = 0; i < sample_count; i++)
		sample_done[i] = 0;
	int i = 0;
	out_output = vec3(0.0);
	for (int k = 0; k < sample_count; k++) {

		vec3 view = rt_current_view(i);
		vec3 view_norm = normalize(view);

		float d = texelFetch(cdepth, pos, i).x;
		vec3 alb = texelFetch(albedo, pos, i).xyz;
		vec3 norm = texelFetch(normal, pos, i).xyz;
		float align = dot(norm, il.sun);
		vec3 direct_light = alb * max(0.0, align) * 2.5;
		direct_light += alb * env_sample((il.view_normal_inv * vec4(norm, 1.0)).xyz);

		vec3 outp = direct_light;
		if (d == 0.0)
			outp = env_sample_novoid((il.view_normal_inv * vec4(view_norm, 1.0)).xyz);

		float count = 0;
		for (int j = 0; j < sample_count; j++) {
			bool same = texelFetch(cdepth, pos, j).x == d;
			sample_done[j] += same ? 1 : 0;
			count += same ? 1.0 : 0.0;
		}
		out_output += outp * count;

		for (int j = 0; j < sample_count; j++) {
			if (sample_done[i] == 0)
				break;
			i++;
			if (i >= sample_count) {
				out_output *= sample_factor;
				return;
			}
		}
	}
}