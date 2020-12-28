#version 460
#extension GL_GOOGLE_include_directive : enable

layout(constant_id = 0) const int sample_count = 1;
layout(constant_id = 1) const float sample_factor = 1.0;

layout(set = 0, binding = 0) uniform Illum {
	mat4 cam_proj;
	mat4 view;
	mat4 view_normal;
	mat4 view_inv;
	mat4 view_normal_inv;
	mat4 last_view;
	mat4 last_view_inv;
	mat4 view_cur_to_last;
	mat4 view_last_to_cur;
	mat4 view_last_to_cur_normal;
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

layout(set = 0, binding = 1) uniform sampler2DMS cdepth;
layout(set = 0, binding = 2) uniform sampler2D depth;
layout(set = 0, binding = 3) uniform sampler2DMS albedo;
layout(set = 0, binding = 4) uniform sampler2DMS normal;
layout(set = 0, binding = 5) uniform sampler2D albedo_resolved;
layout(set = 0, binding = 6) uniform sampler2D normal_resolved;
layout(set = 0, binding = 7) uniform sampler2D last_depth;
layout(set = 0, binding = 8) uniform sampler2D last_albedo_resolved;
layout(set = 0, binding = 9) uniform sampler2D last_normal_resolved;

layout(set = 0, binding = 10) uniform isampler2D last_step;
layout(set = 0, binding = 11) uniform usampler2D last_acc_path_pos;
layout(set = 0, binding = 12) uniform sampler2D last_direct_light;
layout(set = 0, binding = 13) uniform sampler2D last_path_albedo;
layout(set = 0, binding = 14) uniform sampler2D last_path_direct_light;
layout(set = 0, binding = 15) uniform sampler2D last_path_incidence;
layout(set = 0, binding = 16) uniform sampler2D last_output;

layout(location = 0) out int out_step;
layout(location = 1) out uvec4 out_acc_path_pos;
layout(location = 2) out vec3 out_direct_light;
layout(location = 3) out vec3 out_path_albedo;
layout(location = 4) out vec3 out_path_direct_light;
layout(location = 5) out vec3 out_path_incidence;
layout(location = 6) out vec3 out_output;
layout(location = 7) out vec3 out_ms_output;

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
	vec2 size = textureSize(albedo);
	return ((p * 0.5) + 0.5) * size;
}

vec3 rt_pos_view(vec2 pos)
{
	vec2 size = vec2(1.0) / textureSize(albedo);
	float d = texelFetch(depth, ivec2(pos), 0).x;
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

vec3 rt_pos_view(vec2 pos, int samp)
{
	vec2 size = vec2(1.0) / textureSize(albedo);
	float d = texelFetch(cdepth, ivec2(pos), samp).x;
	float z = rt_depth_to_z(d);
	vec2 uv = pos * size;
	vec2 ndc2 = (uv - 0.5) * 2.0;
	ndc2 *= il.cam_ratio;
	ndc2 *= z;
	return vec3(ndc2, z);
}

vec3 rt_current_view(int samp)
{
	return rt_pos_view(gl_FragCoord.xy, samp);
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

bool rt_traceRay(vec3 origin, vec3 dir, vec3 norm, int quality, out vec2 pos)
{
	vec3 p0, p1;
	rt_project_ray(origin, dir, p0, p1);

	vec2 dir2 = p1.xy - p0.xy;
	float dir2_len_ni = length(dir2);
	if (dir2_len_ni == 0.0)
		return false;
	float dir_len = 1.0 / dir2_len_ni;

	float t = 0.0;
	int level = 0;

	float t_max;
	if (!rt_inter_rect_strong(vec2(0.0), vec2(textureSize(albedo)), p0.xy, dir2, dir_len * (1.0 / 64.0), t_max))
		return false;

	vec3 p = p0;
	if (dot(norm, dir) < 0.0) {
		pos = p.xy;
		return true;
	}

	t += dir_len * 2.0;
	p = mix(p0, p1, t);
	const int base_quality = 5;
	//const int quality = 0;
	for (int i = 0; i < 512; i++) {
		float d = textureLod(depth, p.xy * il.depth_size, level == 0 ? 0 : (level + quality)).x;
		if (p.z < d) {
			if (level == 0) {
				pos = p.xy;
				return true;
			}
			level--;
		} else {
			t += dir_len * float(1 << (level + quality));
			if (t >= t_max)
				return false;
			p = mix(p0, p1, t);
			level++;
			level = min(base_quality, level);
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

vec3 last_pos_view(vec2 pos)
{
	float d = textureLod(last_depth, pos * il.depth_size, 0).x;
	float z = rt_depth_to_z(d);
	vec2 uv = pos / il.size;
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
	vec3 nx = vec3(-normal.z, normal.y, normal.x);
	vec3 ny = vec3(normal.y, -normal.z, normal.x);
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
	return env_sample_novoid(dir) * (dir.y > 0.0 ? 1.0 : 0.0);
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

float vec_sum(vec2 vec)
{
	return vec.x + vec.y;
}

float vec_sum(vec3 vec)
{
	return vec.x + vec.y + vec.z;
}

int vec_sum(bvec3 vec)
{
	return (vec.x ? 1 : 0) + (vec.y ? 1 : 0) + (vec.z ? 1 : 0);
}

float sharp_divergence(vec2 pos)
{
	vec2 center = floor(pos) + 0.5;
	float res = length(pos - center);
	if (res < 0.01)
		return 0.0;
	return 1.0;
}

void main(void)
{
	ivec2 pos = ivec2(gl_FragCoord.xy);

	vec3 view = rt_current_view();
	vec3 view_norm = normalize(view);
	vec4 last_view = il.view_cur_to_last * vec4(view, 1.0);
	vec2 last_view_pos = rt_project_point(last_view.xyz).xy;
	ivec2 ilast_view_pos = ivec2(last_view_pos);

	const float repr_dist_tres = 0.5;
	bool repr_success = last_view_pos.x >= 0 && last_view_pos.y >= 0 &&
		last_view_pos.x <= (il.size.x) && last_view_pos.y <= (il.size.y) &&
		length((il.view_inv * vec4(view, 1.0)).xyz - (il.last_view_inv * vec4(last_pos_view(last_view_pos), 1.0)).xyz) < repr_dist_tres;

	int rnd = (hash(int(gl_FragCoord.x)) + hash(int(gl_FragCoord.y))) % 256;
	vec3 alb = texelFetch(albedo_resolved, pos, 0).xyz;
	int last_step = texelFetch(last_step, ilast_view_pos, 0).x;
	uvec4 last_acc_path_pos = texelFetch(last_acc_path_pos, ilast_view_pos, 0).xyzw;
	uvec2 last_acc = last_acc_path_pos.xy;
	vec3 vlast_direct_light = correct_nan(textureLod(last_direct_light, last_view_pos, 0).xyz);
	vec3 last_alb = textureLod(last_albedo_resolved, last_view_pos, 0).xyz;
	if (!repr_success) {
		last_step = 0;
		last_acc = uvec2(0);
	}

	uvec2 uray_origin;
	vec3 ray_origin[sample_count];
	vec3 ray_dir[sample_count];
	vec3 ray_normal[sample_count];
	vec3 ray_albedo[sample_count];
	int ray_query_count = 0;
	int quality;
	if (last_step == 0) {
		uray_origin = uvec2(pos);

		int sample_done[sample_count];
		for (int i = 0; i < sample_count; i++)
			sample_done[i] = 0;
		int i = 0;
		vec3 direct_light = vec3(0.0);
		for (int k = 0; k < sample_count; k++) {

			float d = texelFetch(cdepth, pos, i).x;
			ray_origin[ray_query_count] = rt_current_view(i);
			ray_dir[ray_query_count] = (il.view_normal * vec4(il.rnd_sun[rnd], 1.0)).xyz;
			ray_normal[ray_query_count] = texelFetch(normal, pos, i).xyz;
			ray_query_count++;

			float count = 0;
			for (int j = 0; j < sample_count; j++) {
				bool same = texelFetch(cdepth, pos, j).x == d;
				sample_done[j] += same ? 1 : 0;
				count += same ? 1.0 : 0.0;
			}

			bool done = false;
			for (int j = 0; j < sample_count; j++) {
				if (sample_done[i] == 0)
					break;
				i++;
				if (i >= sample_count) {
					done = true;
					break;
				}
			}
			if (done)
				break;
		}
		quality = 1;
	}
	if (last_step == 1) {
		uray_origin = uvec2(pos);

		int sample_done[sample_count];
		for (int i = 0; i < sample_count; i++)
			sample_done[i] = 0;
		int i = 0;
		for (int k = 0; k < sample_count; k++) {

			float d = texelFetch(cdepth, pos, i).x;
			ray_origin[ray_query_count] = rt_current_view(i);
			vec3 norm = texelFetch(normal, pos, i).xyz;
			ray_dir[ray_query_count] = rnd_diffuse_around_rough(normalize(ray_origin[ray_query_count]), norm, 0.0, rnd);
			ray_normal[ray_query_count] = norm;
			ray_albedo[ray_query_count] = texelFetch(albedo, pos, i).xyz;
			ray_query_count++;

			float count = 0;
			for (int j = 0; j < sample_count; j++) {
				bool same = texelFetch(cdepth, pos, j).x == d;
				sample_done[j] += same ? 1 : 0;
				count += same ? 1.0 : 0.0;
			}

			bool done = false;
			for (int j = 0; j < sample_count; j++) {
				if (sample_done[i] == 0)
					break;
				i++;
				if (i >= sample_count) {
					done = true;
					break;
				}
			}
			if (done)
				break;
		}
		out_path_albedo = ray_albedo[0];
		out_path_direct_light = vlast_direct_light;
		quality = 3;
	}
	if (last_step == 2) {
		uvec2 last_path_pos = last_acc_path_pos.zw;
		vec3 last_end = (il.view_last_to_cur * vec4(last_pos_view(last_path_pos), 1.0)).xyz;
		ray_query_count = 1;
		ray_origin[0] = last_end;
		uray_origin = uvec2(rt_project_point(ray_origin[0]).xy);
		vec3 norm = (il.view_last_to_cur_normal * vec4(textureLod(last_path_incidence, last_view_pos, 0).xyz, 1.0)).xyz;
		ray_dir[0] = rnd_diffuse_around_rough(norm,
			(il.view_last_to_cur_normal * vec4(texelFetch(last_normal_resolved, ilast_view_pos, 0).xyz, 1.0)).xyz, 0.0, rnd);
		ray_normal[0] = norm;
		out_path_albedo = texelFetch(last_path_albedo, ilast_view_pos, 0).xyz;
		out_path_direct_light = texelFetch(last_path_direct_light, ilast_view_pos, 0).xyz;
		quality = 3;
	}

	vec2 ray_pos[sample_count];
	bool ray_success[sample_count];
	for (int i = 0; i < ray_query_count; i++)
		ray_success[i] = rt_traceRay(ray_origin[i], ray_dir[i], ray_normal[i], quality, ray_pos[i]);

	if (last_step == 0) {
		out_step = 1;
		out_acc_path_pos.xy = last_acc;

		int sample_done[sample_count];
		for (int i = 0; i < sample_count; i++)
			sample_done[i] = 0;
		int i = 0;
		vec3 direct_light = vec3(0.0);
		int cur_query = 0;
		for (int k = 0; k < sample_count; k++) {

			float d = texelFetch(cdepth, pos, i).x;
			vec3 norm = texelFetch(normal, pos, i).xyz;
			vec3 alb = texelFetch(albedo, pos, i).xyz;
			float align = dot(norm, il.sun);
			vec3 cur_direct_light = alb * max(0.0, align) * (ray_success[cur_query] ? 0.0 : 1.0) * 2.5;
			if (d == 0.0)
				cur_direct_light = env_sample_novoid((il.view_normal_inv * vec4(view_norm, 1.0)).xyz);

			float count = 0;
			for (int j = 0; j < sample_count; j++) {
				bool same = texelFetch(cdepth, pos, j).x == d;
				sample_done[j] += same ? 1 : 0;
				count += same ? 1.0 : 0.0;
			}
			direct_light += cur_direct_light * count;

			bool done = false;
			for (int j = 0; j < sample_count; j++) {
				if (sample_done[i] == 0)
					break;
				i++;
				if (i >= sample_count) {
					direct_light *= sample_factor;
					done = true;
					break;
				}
			}
			if (done)
				break;
			cur_query++;
		}

		out_direct_light = irradiance_correct_adv(vlast_direct_light, last_alb,
			direct_light, alb, last_acc.x);

		vec3 outp = textureLod(last_output, last_view_pos, 0).xyz;
		out_output = irradiance_correct(outp, last_alb, alb);

		if (sharp_divergence(last_view_pos) > 0.0) {
			if (length(vlast_direct_light - direct_light) > 0.01)
				out_acc_path_pos.x = 0;
		}
	}
	if (last_step >= 1) {
		out_step = ray_success[0] ? 2 : 0;
		out_acc_path_pos.x = min(last_acc.x + (ray_success[0] ? 0 : 1), 65000);
		out_acc_path_pos.y = last_acc.y;
		out_direct_light = irradiance_correct(vlast_direct_light, last_alb, alb);

		out_acc_path_pos.zw = uvec2(ray_pos[0]);
		out_path_incidence = ray_dir[0];
		vec3 foutput = correct_nan(textureLod(last_output, last_view_pos, 0).xyz);

		if (last_step == 1) {
			vec3 path_direct_light = vec3(0.0);
			int sample_done[sample_count];
			for (int i = 0; i < sample_count; i++)
				sample_done[i] = 0;
			int i = 0;
			int query = 0;
			for (int k = 0; k < sample_count; k++) {

				float d = texelFetch(cdepth, pos, i).x;
				vec3 pdl;
				if (ray_success[query])
					pdl = correct_nan(textureLod(last_direct_light, ray_pos[query], 0).xyz) * ray_albedo[query];
				else
					pdl = env_sample(ray_dir[query]) * ray_albedo[query];
				query++;

				float count = 0;
				for (int j = 0; j < sample_count; j++) {
					bool same = texelFetch(cdepth, pos, j).x == d;
					sample_done[j] += same ? 1 : 0;
					count += same ? 1.0 : 0.0;
				}
				path_direct_light += pdl * count;

				bool done = false;
				for (int j = 0; j < sample_count; j++) {
					if (sample_done[i] == 0)
						break;
					i++;
					if (i >= sample_count) {
						done = true;
						break;
					}
				}
				if (done)
					break;
			}
			path_direct_light *= sample_factor;
			out_path_direct_light += path_direct_light;
		} else {
			if (ray_success[0])
				out_path_direct_light += correct_nan(textureLod(last_direct_light, ray_pos[0], 0).xyz) * out_path_albedo;
			else
				out_path_direct_light += env_sample(ray_dir[0]) * out_path_albedo;
		}
		if (ray_success[0]) {
			out_path_albedo *= textureLod(albedo_resolved, ray_pos[0], 0).xyz;
			out_output = irradiance_correct(foutput, last_alb, alb);
		} else
			out_output = irradiance_correct_adv(foutput, last_alb, out_path_direct_light, alb, last_acc.x);
		if (sharp_divergence(last_view_pos) > 0.0) {
			if (length(vlast_direct_light - out_direct_light) > 0.01)
				out_acc_path_pos.x = 0;
		}
	}

	/*float env_count = 0.0;
	for (int i = 0; i < sample_count; i++)
		env_count += texelFetch(cdepth, pos, i).x == 0.0 ? sample_factor : 0.0;
	out_ms_output = correct_nan(out_output) + env_sample_novoid((il.view_normal_inv * vec4(view_norm, 1.0)).xyz) * env_count;*/

	out_output = correct_nan(out_output);
	float max_d = 0.0;
	for (int i = 0; i < sample_count; i++)
		max_d = max(max_d, texelFetch(cdepth, pos, i).x);

	bool env_reset = max_d == 0.0;
	if (env_reset)
		out_output = env_sample_novoid((il.view_normal_inv * vec4(view_norm, 1.0)).xyz);
	out_ms_output = out_output;
	float env_count = 0.0;
	for (int i = 0; i < sample_count; i++)
		env_count += texelFetch(cdepth, pos, i).x == 0.0 ? sample_factor : 0.0;
	//if (!env_reset)
	//	out_ms_output += env_sample_novoid((il.view_normal_inv * vec4(view_norm, 1.0)).xyz) * env_count;
}