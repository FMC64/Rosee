
float tex_diff(float val, float y)
{
	float vadj = y - val;
	if (vadj > 0.0)
		return vadj;
	else if (vadj > -1.0)
		return vadj + 1.0;
	else
		return vadj + 2.0;
}

vec2 tex_3dmap(vec3 pos)
{
	vec2 uvf = fract(pos.xz);
	float yf = fract(pos.y);
	vec2 ouvf = 1.0 - uvf;
	float g10 = tex_diff(ouvf.x + uvf.y, yf);
	float g01 = tex_diff(uvf.x + ouvf.y, yf);
	float g11 = tex_diff(uvf.x + uvf.y, yf);
	return vec2(1, 0) * g10 + vec2(0, 1) * g01 + vec2(1) * g11;
}