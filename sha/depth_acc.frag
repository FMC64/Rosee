#version 460
#extension GL_GOOGLE_include_directive : enable

layout(set = 0, binding = 0) uniform sampler2D depth;

layout(location = 0) out float out_depth;

void main(void)
{
	ivec2 pos = ivec2(gl_FragCoord.xy) * 2;
	out_depth = max(max(max(
		texelFetch(depth, pos, 0).x,
		texelFetch(depth, pos + ivec2(1, 0), 0).x),
		texelFetch(depth, pos + ivec2(1, 1), 0).x),
		texelFetch(depth, pos + ivec2(1, 1), 0).x);
}