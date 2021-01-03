
float rt_depth_to_z(float d)
{
	return il.cam_b / (d -  il.cam_a);
}

float rt_z_to_depth(float z)
{
	return il.cam_b / z + il.cam_a;
}

vec2 rt_ndc_to_ss(vec2 p)
{
	return ((p * 0.5) + 0.5) * il.size;
}

vec3 rt_pos_view(vec2 pos, float depth)
{
	float z = rt_depth_to_z(depth);
	vec2 uv = pos * il.size_inv;
	vec2 ndc2 = (uv - 0.5) * 2.0;
	ndc2 *= il.cam_ratio;
	ndc2 *= z;
	return vec3(ndc2, z);
}

vec3 rt_project_point(vec3 point)
{
	vec4 ph = il.cam_proj * vec4(point, 1.0);
	vec3 res = ph.xyz / ph.w;
	return vec3(rt_ndc_to_ss(res.xy), res.z);
}

int hash(int x)
{
	x += ( x << 10 );
	x ^= ( x >>  6 );
	x += ( x <<  3 );
	x ^= ( x >> 11 );
	x += ( x << 15 );
	return x;
}

const float irradiance_albedo_bias = 0.01;

vec3 output_to_irradiance(vec3 outp, vec3 albedo)
{
	return outp / (albedo + irradiance_albedo_bias);
}

vec3 irradiance_to_output(vec3 irradiance, vec3 albedo)
{
	return irradiance * (albedo + irradiance_albedo_bias);
}

vec3 irradiance_correct(vec3 prev_value, vec3 prev_albedo, vec3 cur_albedo)
{
	vec3 last_irr = output_to_irradiance(prev_value, prev_albedo);
	return irradiance_to_output(last_irr, cur_albedo);
}

vec3 irradiance_correct_adv(vec3 prev_value, vec3 prev_albedo, vec3 cur_value, vec3 cur_albedo, float count)
{
	vec3 last_irr = output_to_irradiance(prev_value, prev_albedo);
	vec3 cur_irr = output_to_irradiance(cur_value, cur_albedo);
	return irradiance_to_output((last_irr * count + cur_irr) / (count + 1.0), cur_albedo);
}

vec3 rnd_diffuse_around(vec3 normal, int rand)
{
	vec3 nx;
	vec3 ny;
	if (abs(normal.x) > abs(normal.y))
		nx = vec3(normal.z, 0, -normal.x) / sqrt(normal.x * normal.x + normal.z * normal.z);
	else
		nx = vec3(0, -normal.z, normal.y) / sqrt(normal.y * normal.y + normal.z * normal.z);
	ny = cross(normal, nx);

	vec3 nz = normal;
	vec3 rvec = il.rnd_diffuse[rand];
	return nx * rvec.x + ny * rvec.y + nz * rvec.z;
}

vec3 rnd_diffuse_around_rough(vec3 i, vec3 normal, float roughness, int rand)
{
	return normalize(mix(rnd_diffuse_around(normal, rand), reflect(i, normal), roughness));
}

vec3 env_sample_novoid(vec3 dir)
{
	const vec3 hor = vec3(80, 120, 180) / 255;
	const vec3 up = vec3(0, 60, 256) / 255;
	float up_ratio = dot(dir, vec3(0.0, 1.0, 0.0));

	up_ratio = 1.0 - up_ratio;
	up_ratio *= up_ratio;
	up_ratio *= up_ratio;
	up_ratio = 1.0 - up_ratio;
	return normalize(hor * (1.0 - up_ratio) + vec3(1.0) * up);
}

vec3 env_sample(vec3 dir)
{
	return env_sample_novoid(dir) * ((dir.y + 1.0) * 0.5);
}

float sharp_divergence(vec2 pos)
{
	vec2 center = floor(pos) + 0.5;
	float res = length(pos - center);
	if (res < 0.01)
		return 0.0;
	return 1.0;
}