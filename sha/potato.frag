#version 460
#extension GL_GOOGLE_include_directive : enable

layout(constant_id = 0) const int sample_count = 1;
layout(constant_id = 1) const float sample_factor = 1.0;

#include "illum.glsl"

layout(set = 0, binding = 1) uniform sampler2D cdepth;
layout(set = 0, binding = 2) uniform sampler2D albedo;
layout(set = 0, binding = 3) uniform sampler2D normal;

layout(location = 0) out vec3 out_output;

#include "rt.glsl"

vec3 rt_pos_view(vec2 pos)
{
	return rt_pos_view(pos, texelFetch(cdepth, ivec2(pos), 0).x);
}

vec3 rt_current_view()
{
	return rt_pos_view(gl_FragCoord.xy);
}

void main(void)
{
	ivec2 pos = ivec2(gl_FragCoord.xy);
	vec3 view = rt_current_view();
	vec3 view_norm = normalize(view);

	vec3 alb = texelFetch(albedo, pos, 0).xyz;
	vec3 norm = texelFetch(normal, pos, 0).xyz;
	float align = dot(norm, il.sun);
	vec3 direct_light = alb * max(0.0, align) * 2.5;
	direct_light += alb * env_sample((il.view_normal_inv * vec4(norm, 1.0)).xyz);

	out_output = direct_light;
	if (texelFetch(cdepth, pos, 0).x == 0.0)
		out_output = env_sample_novoid((il.view_normal_inv * vec4(view_norm, 1.0)).xyz);
}