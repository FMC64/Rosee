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

float tex_diff(float val, float y)
{
	float vadj = y - val;
	if (vadj > 0.0)
		return vadj;
	else if (vadj > -1.0)
		return vadj + 1.0;
	else
		return vadj + 2.0;
}

vec2 tex_3dmap(vec3 pos)
{
	vec2 uvf = fract(pos.xz);
	float yf = fract(pos.y);
	vec2 ouvf = 1.0 - uvf;
	return vec2(tex_diff(uvf.x + uvf.y, yf), tex_diff(ouvf.x + ouvf.y, yf));
}

void main(void)
{
	vec4 t = texture(samplers[p.albedo], tex_3dmap(in_w));
	//vec4 t = vec4(vec3(v), 1.0);
	if (t.w < 0.01)
		discard;
	out_depth = gl_FragCoord.z;
	out_albedo = vec4(t.xyz, 1.0);
	out_normal = normalize(in_n);
}