#version 460
#extension GL_GOOGLE_include_directive : enable

layout(set = 0, binding = 0) uniform Illum {
	vec3 sun;
} il;
layout(set = 0, binding = 1) uniform sampler2D albedo;
layout(set = 0, binding = 2) uniform sampler2D normal;

layout(location = 0) out vec3 out_output;

void main(void)
{
	vec3 alb = texture(albedo, gl_FragCoord.xy).xyz;
	vec3 norm = normalize(texture(normal, gl_FragCoord.xy).xyz);
	float illum = 1.0;//max(dot(norm, -il.sun), 0.0);
	out_output = alb * illum * 1.5;
}