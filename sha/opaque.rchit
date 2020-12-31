#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable

#include "ray_tracing_in.glsl"

void main(void)
{
	rsp.hit = true;
	rsp.albedo = vec3(0.5);
	rsp.normal = vec3(0.0, 1.0, 0.0);
}