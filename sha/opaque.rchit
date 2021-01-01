#version 460
#extension GL_GOOGLE_include_directive : enable

#include "ray_tracing.glsl"

layout(location = 0) rayPayloadInEXT RayPayload rp;

void main(void)
{
	rp.hit = true;
	rp.pos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	rp.albedo = vec3(0.5);
	rp.normal = vec3(0.0, 1.0, 0.0);
}