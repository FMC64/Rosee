#version 460
#extension GL_GOOGLE_include_directive : enable

#include "particle.pc"

layout(location = 0) out vec4 out_wsi;

void main(void)
{
	out_wsi = vec4(p.color, 1.0);
}