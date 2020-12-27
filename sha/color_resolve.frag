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
	float max_depth = -1.0;
	for (int i = 0; i < sample_count; i++) {
		float cur_d = texelFetch(cdepth, pos, i).x;
		if (cur_d > max_depth) {
			max_depth = cur_d;
			out_albedo = texelFetch(albedo, pos, i);
			out_normal = texelFetch(normal, pos, i).xyz;
		}
	}
}