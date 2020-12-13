#version 460
#extension GL_GOOGLE_include_directive : enable

layout(constant_id = 0) const int sample_count = 1;
layout(constant_id = 1) const float sample_factor = 1.0;

layout(set = 0, binding = 0) uniform sampler2DMS cdepth;

layout(location = 0) out float out_depth;

void main(void)
{
	ivec2 pos = ivec2(gl_FragCoord.xy);
	out_depth = 0.1;
}