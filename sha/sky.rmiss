#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable

#include "ray_tracing_in.glsl"

void main(void)
{
	rsp.hit = false;
}