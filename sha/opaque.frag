#version 460
#extension GL_GOOGLE_include_directive : enable

#include "0_frag.set"

layout(push_constant) uniform PushConstant {
	int albedo;
} p;

layout(location = 0) in vec3 in_n;
layout(location = 1) in vec3 in_w;

layout(location = 0) out float out_depth;
layout(location = 1) out vec4 out_albedo;
layout(location = 2) out vec3 out_normal;

void main(void)
{
	vec2 uv = fract(in_w.xz);
	vec4 t = texture(samplers[p.albedo], uv);
	if (t.w < 0.01)
		discard;
	out_depth = gl_FragCoord.z;
	out_albedo = vec4(t.xyz, 1.0);
	out_normal = normalize(in_n);
}