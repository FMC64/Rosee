#version 460
#extension GL_GOOGLE_include_directive : enable

layout(set = 0, binding = 0) uniform Illum {
	vec3 sun;
} il;
layout(set = 0, binding = 1) uniform sampler2DMS albedo;
layout(set = 0, binding = 2) uniform sampler2DMS normal;

layout(location = 0) out vec3 out_output;

void main(void)
{
	out_output = vec3(0.0);

	for (int i = 0; i < 8; i++) {
		vec3 alb = texelFetch(albedo, ivec2(gl_FragCoord.xy), i).xyz;
		vec3 norm = normalize(texelFetch(normal, ivec2(gl_FragCoord.xy), i).xyz);
		float illum = max(dot(norm, il.sun), 0.05);
		out_output += alb * illum * 1.5;
	}
	out_output *= 0.125;
}