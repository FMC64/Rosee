#version 460
#extension GL_GOOGLE_include_directive : enable

#include "ray_tracing.glsl"

layout(location = 0) rayPayloadInEXT RayPayload rp;

hitAttributeEXT vec2 baryCoord;

void main(void)
{
	rp.hit = true;
	rp.pos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

	Instance ins = instances.instances[gl_InstanceCustomIndexEXT];
	Vertex_pnu v = vertex_read_pnu(ins.model, gl_PrimitiveID, baryCoord);
	Material_albedo m = materials_albedo.materials[ins.material];
	rp.albedo = texture(samplers[m.albedo], v.u).xyz;
	rp.normal = v.n;
}