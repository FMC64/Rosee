
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

uint probe_offset(ivec2 pos, uint layer)
{
	return (pos.y * il.probe_extent.x + pos.x) * probe_layer_count + layer;
}

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

uint oct_fill_mirror(ivec2 pos, out ivec2 offs[8])
{
	uint off_count = 0;
	if (pos.x == 0)
		offs[off_count++] = ivec2(0, probe_diffuse_size_next - pos.y);
	if (pos.y == 0)
		offs[off_count++] = ivec2(probe_diffuse_size_next - pos.x, 0);
	if (pos.x == probe_diffuse_size_next - 1)
		offs[off_count++] = ivec2(probe_diffuse_size - 1, probe_diffuse_size_next - pos.y);
	if (pos.y == probe_diffuse_size_next - 1)
		offs[off_count++] = ivec2(probe_diffuse_size_next - pos.x, probe_diffuse_size - 1);

	if (pos.x == 0 && pos.y == 0)
		offs[off_count++] = ivec2(probe_diffuse_size - 1, probe_diffuse_size - 1);
	if (pos.x == probe_diffuse_size_next - 1 && pos.y == 0)
		offs[off_count++] = ivec2(0, probe_diffuse_size - 1);
	if (pos.x == 0 && pos.y == probe_diffuse_size_next - 1)
		offs[off_count++] = ivec2(probe_diffuse_size - 1, 0);
	if (pos.x == probe_diffuse_size_next - 1 && pos.y == probe_diffuse_size_next - 1)
		offs[off_count++] = ivec2(0, 0);
	return off_count;
}

vec3 probe_pos_to_dir(ivec2 pos)
{
	return oct_proj_inv((vec2(pos) + 0.5) / vec2(probe_diffuse_size_next));
}