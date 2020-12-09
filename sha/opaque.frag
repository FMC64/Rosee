#version 460
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 out_wsi;

void main(void)
{
	out_wsi = vec4(vec3(gl_FragCoord.z), 1.0);
}