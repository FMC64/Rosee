// origin is start of the ray, camera space
// dir is the direction of the ray, camera space
void rt_project_ray(vec3 origin, vec3 dir, out vec3 ss_p0, out vec3 ss_p1)
{
	if (dir.z == 0.0) {
		ss_p0 = rt_project_point(origin);
		ss_p1 = rt_project_point(origin + dir * 16000.0);	// max screen size is 16k because of that 'trick'
		return;
	}
	float len;
	if (dir.z > 0.0)
		len = (il.cam_near - origin.z) / dir.z;
	else
		len = (il.cam_far - origin.z) / dir.z;
	vec3 end = origin + dir * len;
	ss_p0 = rt_project_point(origin);
	ss_p1 = rt_project_point(end);
}

const float rt_inf = 1.0 / 0.0;

float rt_inter_rect(vec2 tl, vec2 br, vec2 p, vec2 d)
{
	return min(
		d.x == 0.0 ? rt_inf : (d.x > 0.0 ? (br.x - p.x) / d.x : (tl.x - p.x) / d.x),
		d.y == 0.0 ? rt_inf : (d.y > 0.0 ? (br.y - p.y) / d.y : (tl.y - p.y) / d.y)
	);
}
bool rt_inter_rect_strong(vec2 tl, vec2 br, vec2 p, vec2 d, float bias, out float res)
{
	res = rt_inter_rect(tl, br, p, d);

	vec2 inter = p + (res - bias) * d;
	return inter.x >= tl.x && inter.y >= tl.y && inter.x <= br.x && inter.y <= br.y;
}

bool anynan(vec3 vec)
{
	bvec3 n = isnan(vec);
	return n.x || n.y || n.z;
}

vec3 correct_nan(vec3 vec)
{
	if (anynan(vec))
		return vec3(0.0);
	else
		return vec;
}