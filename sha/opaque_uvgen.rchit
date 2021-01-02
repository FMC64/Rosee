#version 460
#extension GL_GOOGLE_include_directive : enable

#include "ray_tracing.glsl"

layout(location = 0) rayPayloadInEXT RayPayload rp;

hitAttributeEXT vec2 baryCoord;

#include "tex.glsl"

void main(void)
{
	rp.hit = true;
	rp.pos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

	Instance ins = instances.instances[gl_InstanceCustomIndexEXT];
	Vertex_pn v = vertex_read_pn_i16(ins.model, gl_PrimitiveID, baryCoord);
	Material_albedo m = materials_albedo.materials[ins.material];
	rp.albedo = texture(samplers[m.albedo], tex_3dmap((il.view_inv * vec4(rp.pos, 1.0)).xyz)).xyz;
	rp.normal = normalize(v.n);
}