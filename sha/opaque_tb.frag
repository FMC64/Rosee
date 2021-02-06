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
layout(location = 2) out vec4 out_normal;
layout(location = 3) out vec3 out_normal_geom;

void main(void)
{
	vec2 uv = vec2(in_u.x, -in_u.y) * 32.0;
	vec4 te = texture(samplers[p.albedo], uv);
	if (te.w < 0.01)
		discard;
	out_depth = gl_FragCoord.z;
	out_albedo = vec4(te.xyz, 1.0);
	vec3 n = normalize(in_n);
	vec3 t = normalize(in_t);
	vec3 b = normalize(in_b);
	vec3 nmap = texture(samplers[p.albedo + 1], uv).xyz * 2.0 - 1.0;
	nmap.y *= -1.0;
	nmap = normalize(nmap);
	out_normal = vec4(t * nmap.x + b * nmap.y + n * nmap.z, texture(samplers[p.albedo + 3], uv).x);
	out_normal_geom = n;
}