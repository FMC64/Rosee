#version 460
#extension GL_GOOGLE_include_directive : enable

layout(constant_id = 0) const int sample_count = 1;

layout(set = 0, binding = 0) uniform sampler2DMS cdepth;
layout(set = 0, binding = 1) uniform sampler2DMS albedo;
layout(set = 0, binding = 2) uniform sampler2DMS normal;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec3 out_normal;

void main(void)
{
	ivec2 pos = ivec2(gl_FragCoord.xy);
	float min_depth = 2.0;
	float min_depth_l = 2.0;
	int res_i = 0;
	int res_i_l = 0;
	for (int i = 0; i < sample_count; i++) {
		float d = texelFetch(cdepth, pos, i).x;
		if (d < min_depth) {
			min_depth = d;
			res_i = i;
		}
		if ((d == 0.0 ? 2.0 : d) < min_depth_l) {
			min_depth_l = d;
			res_i_l = i;
		}
	}
	if (min_depth_l != 2.0) {
		out_albedo = texelFetch(albedo, pos, res_i_l);
		out_normal = texelFetch(normal, pos, res_i_l).xyz;
	} else {
		out_albedo = texelFetch(albedo, pos, res_i);
		out_normal = texelFetch(normal, pos, res_i).xyz;
	}
}