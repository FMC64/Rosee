#version 460
#extension GL_GOOGLE_include_directive : enable

layout(constant_id = 0) const int sample_count = 1;
layout(constant_id = 1) const float sample_factor = 1.0;

#include "illum.glsl"

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

#include "rt.glsl"

vec3 rt_pos_view(vec2 pos)
{
	return rt_pos_view(pos, texelFetch(depth, ivec2(pos), 0).x);
}

vec3 rt_pos_view(vec2 pos, int samp)
{
	return rt_pos_view(pos, texelFetch(cdepth, ivec2(pos), samp).x);
}

vec3 rt_current_view()
{
	return rt_pos_view(gl_FragCoord.xy);
}

vec3 rt_current_view(int samp)
{
	return rt_pos_view(gl_FragCoord.xy, samp);
}

#include "ssgi.glsl"

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

vec3 last_pos_view(vec2 pos)
{
	return rt_pos_view(pos, textureLod(last_depth, pos * il.depth_size, 0).x);
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

	int rnd = (hash(int(gl_FragCoord.x)) + hash(int(gl_FragCoord.y))) % 8192;
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
	bool ray_inhibit[sample_count];
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
			ray_inhibit[ray_query_count] = d == 0.0;
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
			ray_inhibit[ray_query_count] = d == 0.0;
			if (!ray_inhibit[ray_query_count]) {
				ray_origin[ray_query_count] = rt_current_view(i);
				vec3 norm = texelFetch(normal, pos, i).xyz;
				ray_dir[ray_query_count] = rnd_diffuse_around_rough(normalize(ray_origin[ray_query_count]), norm, 0.0, rnd);
				ray_normal[ray_query_count] = norm;
				ray_albedo[ray_query_count] = texelFetch(albedo, pos, i).xyz;
			}
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
		ray_inhibit[0] = false;
		out_path_albedo = texelFetch(last_path_albedo, ilast_view_pos, 0).xyz;
		out_path_direct_light = texelFetch(last_path_direct_light, ilast_view_pos, 0).xyz;
		quality = 3;
	}

	vec2 ray_pos[sample_count];
	bool ray_success[sample_count];
	for (int i = 0; i < ray_query_count; i++)
		ray_success[i] = ray_inhibit[i] ? false : rt_traceRay(ray_origin[i], ray_dir[i], ray_normal[i], quality, ray_pos[i]);

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
				if (ray_inhibit[query])
					pdl = env_sample_novoid((il.view_normal_inv * vec4(view_norm, 1.0)).xyz);
				else {
					if (ray_success[query])
						pdl = correct_nan(textureLod(last_direct_light, ray_pos[query], 0).xyz) * ray_albedo[query];
					else
						pdl = env_sample(ray_dir[query]) * ray_albedo[query];
				}
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

	out_output = correct_nan(out_output);
	bool env_full = true;
	for (int i = 0; i < sample_count; i++)
		if (texelFetch(cdepth, pos, i).x != 0.0)
			env_full = false;
	if (env_full)
		out_output = env_sample_novoid((il.view_normal_inv * vec4(view_norm, 1.0)).xyz);
}