#version 460
#extension GL_GOOGLE_include_directive : enable

#include "0_frag.set"

layout(push_constant) uniform PushConstant {
	int albedo;
} p;

layout(location = 0) in vec3 in_n;
layout(location = 1) in vec3 in_t;
layout(location = 2) in vec3 in_b;
layout(location = 3) in vec2 in_u;

layout(location = 0) out float out_depth;
layout(location = 1) out vec4 out_albedo;
layout(location = 2) out vec3 out_normal;
layout(location = 3) out vec3 out_normal_geom;

void main(void)
{
	vec4 te = texture(samplers[p.albedo], vec2(in_u.x, -in_u.y));
	if (te.w < 0.01)
		discard;
	out_depth = gl_FragCoord.z;
	out_albedo = vec4(te.xyz, 1.0);
	vec3 n = normalize(in_n);
	vec3 t = normalize(in_t);
	vec3 b = normalize(in_b);
	vec3 nmap = texture(samplers[p.albedo + 1], vec2(in_u.x, -in_u.y) * 32.0).xyz * 2.0 - 1.0;
	nmap.xy *= vec2(-1.0);
	nmap.z *= 0.6;
	nmap = normalize(nmap);
	out_normal = t * nmap.x + b * nmap.y + n * nmap.z;
	out_normal_geom = n;
}