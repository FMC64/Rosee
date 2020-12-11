#version 460
#extension GL_GOOGLE_include_directive : enable

#include "0_frag.set"

layout(location = 0) in vec2 in_u;

layout(location = 0) out vec4 out_wsi;

void main(void)
{
	vec4 t = texture(samplers[0], vec2(in_u.x, -in_u.y));
	if (t.w < 0.01)
		discard;
	out_wsi = vec4(t.xyz, 1.0);
}