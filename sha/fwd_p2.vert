#version 460
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec2 in_p;

void main(void)
{
	gl_Position = vec4(in_p, 0.0, 1.0);
}
