#version 460
#extension GL_GOOGLE_include_directive : enable

#include "0_frag.set"

layout(location = 0) in vec2 in_u;
layout(location = 1) in vec3 in_n;

layout(location = 0) out float out_depth;
layout(location = 1) out vec4 out_albedo;
layout(location = 2) out vec3 out_normal;

void main(void)
{
	vec4 t = texture(samplers[0], vec2(in_u.x, -in_u.y));
	if (t.w < 0.01)
		discard;
	out_depth = gl_FragCoord.z;
	out_albedo = vec4(t.xyz, 1.0);
	out_normal = in_n;
}