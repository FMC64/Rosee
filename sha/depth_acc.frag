#version 460
#extension GL_GOOGLE_include_directive : enable

layout(set = 0, binding = 0) uniform sampler2D depth;

layout(location = 0) out float out_depth;

void main(void)
{
	ivec2 pos = ivec2(gl_FragCoord.xy);
	out_depth = 0.5;
}