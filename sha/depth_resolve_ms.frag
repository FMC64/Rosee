#version 460
#extension GL_GOOGLE_include_directive : enable

layout(constant_id = 0) const int sample_count = 1;

layout(set = 0, binding = 0) uniform sampler2DMS cdepth;

layout(location = 0) out float out_depth;

void main(void)
{
	ivec2 pos = ivec2(gl_FragCoord.xy);
	out_depth = 2.0;
	float depth_l = 2.0;
	for (int i = 0; i < sample_count; i++) {
		float d = texelFetch(cdepth, pos, i).x;
		out_depth = min(out_depth, d);
		depth_l = min(depth_l, d == 0.0 ? 2.0 : d);
	}
	if (depth_l != 2.0)
		out_depth = depth_l;
}