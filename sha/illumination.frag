#version 460
#extension GL_GOOGLE_include_directive : enable

layout(set = 0, binding = 0) uniform sampler2D albedo;
layout(set = 0, binding = 1) uniform sampler2D normal;

layout(location = 0) out vec3 out_output;

void main(void)
{
	out_output = texture(albedo, gl_FragCoord.xy).xyz;
}