#version 460
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 in_color;
layout(location = 0) out vec4 out_wsi;

void main(void)
{
	out_wsi = vec4(in_color, 1.0);
}