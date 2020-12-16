#version 460
#extension GL_GOOGLE_include_directive : enable

layout(constant_id = 0) const int sample_count = 1;
layout(constant_id = 1) const float sample_factor = 1.0;

layout(set = 0, binding = 0) uniform Illum {
	mat4 cam_proj;
	mat4 view;
	mat4 view_normal;
	mat4 view_inv;
	mat4 last_view;
	mat4 last_view_inv;
	mat4 cam_cur_to_last;
	vec3 rnd_sun[256];
	vec3 rnd_diffuse[256];
	vec3 sun;
	vec2 size;
	vec2 size_inv;
	vec2 depth_size;
	vec2 cam_ratio;
	float cam_near;
	float cam_far;
	float cam_a;
	float cam_b;
} il;
layout(set = 0, binding = 1) uniform sampler2D cdepth;
layout(set = 0, binding = 2) uniform sampler2D depth;
layout(set = 0, binding = 3) uniform sampler2D albedo;
layout(set = 0, binding = 4) uniform sampler2D normal;
layout(set = 0, binding = 5) uniform sampler2D last_depth;
layout(set = 0, binding = 6) uniform sampler2D last_albedo;

layout(set = 0, binding = 7) uniform isampler2D last_step;
layout(set = 0, binding = 8) uniform isampler2D last_acc;
layout(set = 0, binding = 9) uniform sampler2D last_direct_light;
layout(set = 0, binding = 10) uniform usampler2D last_path_pos;
layout(set = 0, binding = 11) uniform sampler2D last_path_albedo;
layout(set = 0, binding = 12) uniform sampler2D last_path_direct_light;
layout(set = 0, binding = 13) uniform sampler2D last_output;

layout(location = 0) out int out_step;
layout(location = 1) out int out_acc;
layout(location = 2) out vec3 out_direct_light;
layout(location = 3) out uvec2 out_path_pos;
layout(location = 4) out vec3 out_path_albedo;
layout(location = 5) out vec3 out_path_direct_light;
layout(location = 6) out vec3 out_output;

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
	vec2 size = textureSize(albedo, 0);
	return ((p * 0.5) + 0.5) * size;
}

vec3 rt_pos_view(vec2 pos)
{
	vec2 size = vec2(1.0) / textureSize(albedo, 0);
	float d = texelFetch(cdepth, ivec2(pos), 0).x;
	float z = rt_depth_to_z(d);
	vec2 uv = pos * size;
	vec2 ndc2 = (uv - 0.5) * 2.0;
	ndc2 *= il.cam_ratio;
	ndc2 *= z;
	return vec3(ndc2, z);
}

vec3 rt_current_view()
{
	return rt_pos_view(gl_FragCoord.xy);
}

vec3 rt_project_point(vec3 point)
{
	vec4 ph = il.cam_proj * vec4(point, 1.0);
	vec3 res = ph.xyz / ph.w;
	return vec3(rt_ndc_to_ss(res.xy), res.z);
}

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

bool rt_traceRay(vec3 origin, vec3 dir, out vec2 pos)
{
	vec3 p0, p1;
	rt_project_ray(origin, dir, p0, p1);

	vec3 normal = normalize(texelFetch(normal, ivec2(p0.xy), 0).xyz);

	vec2 dir2 = p1.xy - p0.xy;
	float dir2_len_ni = length(dir2);
	if (dir2_len_ni == 0.0)
		return false;
	float dir_len = 1.0 / dir2_len_ni;

	float t = 0.0;
	int level = 0;

	float t_max;
	if (!rt_inter_rect_strong(vec2(0.0), vec2(textureSize(albedo, 0) - 1), p0.xy, dir2, dir_len * (1.0 / 64.0), t_max))
		return false;

	vec3 p = p0;
	if (dot(normal, dir) < 0.0) {
		pos = p.xy;
		return true;
	}

	t += dir_len * 2.0;
	p = mix(p0, p1, t);
	for (int i = 0; i < 512; i++) {
		float d = textureLod(depth, p.xy * il.depth_size, level).x;
		if (p.z <= d) {
			if (level == 0) {
				pos = p.xy;
				return true;
			}
			level--;
		} else {
			t += dir_len * float(1 << level);
			if (t >= t_max)
				return false;
			p = mix(p0, p1, t);
			level++;
			level = min(5, level);
		}
	}
	return false;
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

vec3 last_pos_view(vec2 pos, vec2 size)
{
	float d = textureLod(last_depth, pos * il.depth_size, 0).x;
	float z = rt_depth_to_z(d);
	vec2 uv = pos / size;
	vec2 ndc2 = (uv - 0.5) * 2.0;
	ndc2 *= il.cam_ratio;
	ndc2 *= z;
	return vec3(ndc2, z);
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

vec3 rnd_diffuse_around(vec3 normal, int rand)
{
	vec3 nx = vec3(-normal.z, normal.y, normal.x);
	vec3 ny = vec3(normal.y, -normal.z, normal.x);
	vec3 nz = normal;
	vec3 rvec = il.rnd_diffuse[rand];
	return nx * rvec.x + ny * rvec.y + nz * rvec.z;
}

void main(void)
{
	ivec2 pos = ivec2(gl_FragCoord.xy);

	vec3 view = rt_current_view();
	vec4 last_view = il.cam_cur_to_last * vec4(view, 1.0);
	vec2 last_view_pos = rt_project_point(last_view.xyz).xy;
	ivec2 ilast_view_pos = ivec2(last_view_pos);

	const float repr_dist_tres = 0.5;
	bool repr_success = last_view_pos.x >= 0 && last_view_pos.y >= 0 &&
		last_view_pos.x <= (il.size.x - 1) && last_view_pos.y <= (il.size.y - 1) &&
		length((il.view_inv * vec4(view, 1.0)).xyz - (il.last_view_inv * vec4(last_pos_view(last_view_pos, il.size), 1.0)).xyz) < repr_dist_tres &&
		texture(depth, gl_FragCoord.xy * il.depth_size).x < 0.9999999;

	int rnd = (hash(int(gl_FragCoord.x)) + hash(int(gl_FragCoord.y))) % 256;
	int last_step = texelFetch(last_step, ilast_view_pos, 0).x;
	int last_acc = texelFetch(last_acc, ilast_view_pos, 0).x;
	if (!repr_success) {
		last_step = 0;
		last_acc = 0;
	}

	vec3 ray_origin;
	vec3 ray_dir;
	if (last_step == 0) {
		ray_origin = view;
		ray_dir = (il.view_normal * vec4(il.rnd_sun[rnd], 1.0)).xyz;
	}
	/*if (last_step == 1) {
		ray_origin = view;
		//ray_dir = ;
	}*/

	/*float d = texelFetch(cdepth, pos, 0).x;
	vec3 alb = texelFetch(albedo, pos, 0).xyz;
	vec3 norm = normalize(texelFetch(normal, pos, 0).xyz);
	float align = dot(norm, il.sun);*/

	vec2 ray_pos;
	bool ray_success = rt_traceRay(ray_origin, ray_dir, ray_pos);

	if (last_step == 0) {
		out_step = 0;
		vec3 alb = texelFetch(albedo, pos, 0).xyz;
		vec3 norm = normalize(texelFetch(normal, pos, 0).xyz);
		float align = dot(norm, il.sun);
		out_direct_light = alb * align * (ray_success ? 0.0 : 1.0) * 2.5;
		out_output = out_direct_light;
	}

	/*float illum = max(align, 0.05);
	vec3 outp = alb * illum * 2.5;
	if (repr_success) {
		vec3 last_irr = output_to_irradiance(texture(last_output, last_view_pos * il.size_inv).xyz, texture(last_albedo, last_view_pos * il.size_inv).xyz);
		vec3 cur_irr = output_to_irradiance(outp, alb);
		outp = irradiance_to_output(mix(last_irr, cur_irr, 0.01), alb);
	}*/

	out_acc = 0;
	//out_direct_light = vec3(0.0);
	out_path_pos = uvec2(0);
	out_path_albedo = vec3(0.0);
	//out_path_direct_light = vec3(0.0);
	//out_output = outp;
}