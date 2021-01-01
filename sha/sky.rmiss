#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable

#include "ray_tracing.glsl"

layout(location = 0) rayPayloadInEXT RayPayload rp;

void main(void)
{
	rp.hit = false;
}