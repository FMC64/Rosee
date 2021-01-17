layout(set = 0, binding = 2) uniform sampler2D cdepth;
layout(set = 0, binding = 3) uniform sampler2D albedo;
layout(set = 0, binding = 4) uniform sampler2D normal;

struct Probe {
	vec3 pos;
	ivec2 ipos;
	float depth;
};

layout(constant_id = 2) const uint probe_layer_count = 1;
layout(constant_id = 3) const int probe_size_l2 = 1;
layout(constant_id = 4) const int probe_size = 1;
layout(constant_id = 5) const int probe_diffuse_size = 1;
layout(constant_id = 6) const int probe_max_bounces = 1;
const int probe_diffuse_size_next = probe_diffuse_size - 2;

layout(set = 0, binding = 5) buffer Probes {
	Probe probes[];
} probes_pos;

vec2 oct_flip(vec2 p)
{
	return (vec2(1.0) - abs(vec2(p.y, p.x))) * sign(p);
}

vec2 oct_proj(vec3 d)
{
	d /= dot(vec3(1.0), abs(d));
	vec2 res = vec2(d.x, d.z);
	if (d.y < 0.0)
		res = oct_flip(res);
	res *= 0.5 + vec2(0.5);
	return res;
}

vec3 oct_proj_inv(vec2 p)
{
	p *= 2.0 - vec2(1.0);
	bool is_neg = abs(p.x) + abs(p.y) > 1.0;
	if (is_neg)
		p = oct_flip(p);
	vec3 res = vec3(p.x, 1.0 - abs(p.x) - abs(p.y), p.y);
	if (is_neg)
		res.y = -res.y;
	return res;
}