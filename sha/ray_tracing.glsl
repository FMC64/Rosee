struct RayPayload {
	bool hit;
	vec3 pos;
	vec3 albedo;
	vec3 normal;
};

struct Instance {
	uint model;
	uint material;
};

struct Vertex_pnu {
	vec3 p;
	vec3 n;
	vec2 u;
};

struct Vertex_pn {
	vec3 p;
	vec3 n;
};

struct Material_albedo {
	uint albedo;
};

#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_16bit_storage : enable

layout(set = 1, binding = 0) uniform sampler2D samplers[];

layout(scalar, set = 1, binding = 1) readonly buffer Instances {
	Instance instances[];
} instances;

layout(scalar, set = 1, binding = 2) readonly buffer Models_pnu {
	Vertex_pnu vertices[];
} models_pnu[];

layout(scalar, set = 1, binding = 3) readonly buffer Models_pn_i16_v {
	Vertex_pnu vertices[];
} models_pn_i16_v[];

layout(scalar, set = 1, binding = 4) readonly buffer Models_pn_i16_i {
	uint16_t indices[];
} models_pn_i16_i[];

layout(scalar, set = 1, binding = 5) readonly buffer Materials_albedo {
	Material_albedo materials[];
} materials_albedo[];