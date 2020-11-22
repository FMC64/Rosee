#version 460
#extension GL_GOOGLE_include_directive : enable

#include "particle.pc"

layout(location = 0) in vec2 in_pos;

void main(void)
{
	gl_Position = vec4(p.pos, 0, 1.0);
	gl_PointSize = p.size;
}
