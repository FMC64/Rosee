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