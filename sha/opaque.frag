#version 460
#extension GL_GOOGLE_include_directive : enable

#include "0_frag.set"

layout(push_constant) uniform PushConstant {
	int albedo;
} p;

layout(location = 0) in vec3 in_n;
layout(location = 1) in vec2 in_u;

layout(location = 0) out float out_depth;
layout(location = 1) out vec4 out_albedo;
layout(location = 2) out vec4 out_normal;
layout(location = 3) out vec3 out_normal_geom;

void main(void)
{
	//vec4 t = texture(samplers[p.albedo], vec2(in_u.x, -in_u.y));
	vec4 t = vec4(vec3(0.5), 1.0);
	if (t.w < 0.01)
		discard;
	out_depth = gl_FragCoord.z;
	out_albedo = vec4(t.xyz, 1.0);
	vec3 normal = normalize(in_n);
	out_normal = vec4(normal, 1.0);
	out_normal_geom = normal;
}