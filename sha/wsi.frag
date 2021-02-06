#version 460
#extension GL_GOOGLE_include_directive : enable

layout(set = 0, binding = 0) uniform sampler2D outp;

layout(location = 0) out vec4 out_wsi;

void main(void)
{
	out_wsi = vec4(texelFetch(outp, ivec2(gl_FragCoord.xy), 0).xyz, 1.0) * 1.0;
}