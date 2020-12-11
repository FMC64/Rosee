#version 460
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 out_wsi;

void main(void)
{
	out_wsi = vec4(0.5, 0.0, 0.0, 1.0);
}