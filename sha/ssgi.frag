#version 460
#extension GL_GOOGLE_include_directive : enable

layout(constant_id = 0) const int sample_count = 1;
layout(constant_id = 1) const float sample_factor = 1.0;

#include "illum.glsl"

layout(set = 0, binding = 1) uniform sampler2D depth;
layout(set = 0, binding = 2) uniform sampler2D albedo;
layout(set = 0, binding = 3) uniform sampler2D normal;
layout(set = 0, binding = 4) uniform sampler2D last_depth;
layout(set = 0, binding = 5) uniform sampler2D last_albedo;
layout(set = 0, binding = 6) uniform sampler2D last_normal;

layout(set = 0, binding = 7) uniform isampler2D last_step;
layout(set = 0, binding = 8) uniform usampler2D last_acc_path_pos;
layout(set = 0, binding = 9) uniform sampler2D last_direct_light;
layout(set = 0, binding = 10) uniform sampler2D last_path_albedo;
layout(set = 0, binding = 11) uniform sampler2D last_path_direct_light;
layout(set = 0, binding = 12) uniform sampler2D last_path_incidence;
layout(set = 0, binding = 13) uniform sampler2D last_output;

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

vec3 rt_current_view()
{
	return rt_pos_view(gl_FragCoord.xy);
}

#include "ssgi.glsl"

bool rt_traceRay(vec3 origin, vec3 dir, int quality, out vec2 pos)
{
	vec3 p0, p1;
	rt_project_ray(origin, dir, p0, p1);

	vec3 normal = texelFetch(normal, ivec2(p0.xy), 0).xyz;

	vec2 dir2 = p1.xy - p0.xy;
	float dir2_len_ni = length(dir2);
	if (dir2_len_ni == 0.0)
		return false;
	float dir_len = 1.0 / dir2_len_ni;

	float t = 0.0;
	int level = 0;

	float t_max;
	if (!rt_inter_rect_strong(vec2(0.0), vec2(textureSize(albedo, 0)), p0.xy, dir2, dir_len * (1.0 / 64.0), t_max))
		return false;

	vec3 p = p0;
	if (dot(normal, dir) < 0.0) {
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
	if (sharp_divergence(last_view_pos) == 0.0)
		last_view_pos = vec2(ilast_view_pos) + 0.5;

	const float repr_dist_tres = 0.5;
	bool repr_success = last_view_pos.x >= 0 && last_view_pos.y >= 0 &&
		last_view_pos.x <= (il.size.x) && last_view_pos.y <= (il.size.y) &&
		length((il.view_inv * vec4(view, 1.0)).xyz - (il.last_view_inv * vec4(last_pos_view(last_view_pos), 1.0)).xyz) < repr_dist_tres &&
		texelFetch(depth, pos, 0).x < 0.9999999;

	int rnd = (hash(int(gl_FragCoord.x)) + hash(int(gl_FragCoord.y))) % 256;
	vec3 alb = texelFetch(albedo, pos, 0).xyz;
	int last_step = texelFetch(last_step, ilast_view_pos, 0).x;
	uvec4 last_acc_path_pos = texelFetch(last_acc_path_pos, ilast_view_pos, 0).xyzw;
	uvec2 last_acc = last_acc_path_pos.xy;
	vec3 vlast_direct_light = correct_nan(textureLod(last_direct_light, last_view_pos, 0).xyz);
	vec3 last_alb = textureLod(last_albedo, last_view_pos, 0).xyz;
	if (!repr_success) {
		last_step = 0;
		last_acc = uvec2(0);
	}

	uvec2 uray_origin;
	vec3 ray_origin;
	vec3 ray_dir;
	int quality;
	if (last_step == 0) {
		uray_origin = uvec2(pos);
		ray_origin = view;
		ray_dir = (il.view_normal * vec4(il.rnd_sun[rnd], 1.0)).xyz;
		quality = 1;
	}
	if (last_step == 1) {
		uray_origin = uvec2(pos);
		ray_origin = view;
		ray_dir = rnd_diffuse_around_rough(view_norm, texelFetch(normal, pos, 0).xyz, 0.0, rnd);
		out_path_albedo = alb;
		out_path_direct_light = vlast_direct_light;
		quality = 3;
	}
	if (last_step == 2) {
		uvec2 last_path_pos = last_acc_path_pos.zw;
		vec3 last_end = (il.view_last_to_cur * vec4(last_pos_view(last_path_pos), 1.0)).xyz;
		ray_origin = last_end;
		uray_origin = uvec2(rt_project_point(ray_origin).xy);
		ray_dir = rnd_diffuse_around_rough((il.view_last_to_cur_normal * vec4(textureLod(last_path_incidence, last_view_pos, 0).xyz, 1.0)).xyz,
			(il.view_last_to_cur_normal * vec4(texelFetch(last_normal, ilast_view_pos, 0).xyz, 1.0)).xyz, 0.0, rnd);
		out_path_albedo = texelFetch(last_path_albedo, ilast_view_pos, 0).xyz;
		out_path_direct_light = texelFetch(last_path_direct_light, ilast_view_pos, 0).xyz;
		quality = 3;
	}

	vec2 ray_pos;
	bool ray_success = rt_traceRay(ray_origin, ray_dir, quality, ray_pos);

	if (last_step == 0) {
		out_step = 1;
		out_acc_path_pos.xy = last_acc;
		vec3 norm = texelFetch(normal, pos, 0).xyz;
		float align = dot(norm, il.sun);
		vec3 direct_light = alb * max(0.0, align) * (ray_success ? 0.0 : 1.0) * 2.5;
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
		out_step = ray_success ? 2 : 0;
		out_acc_path_pos.x = min(last_acc.x + (ray_success ? 0 : 1), 65000);
		out_acc_path_pos.y = last_acc.y;
		out_direct_light = irradiance_correct(vlast_direct_light, last_alb, alb);

		out_acc_path_pos.zw = uvec2(ray_pos);
		out_path_incidence = ray_dir;
		vec3 foutput = correct_nan(textureLod(last_output, last_view_pos, 0).xyz);
		if (ray_success) {
			out_path_direct_light += correct_nan(textureLod(last_direct_light, ray_pos, 0).xyz) * out_path_albedo;	// bug with ray_pos in wrong frame
			out_path_albedo *= textureLod(albedo, ray_pos, 0).xyz;
			out_output = irradiance_correct(foutput, last_alb, alb);
		} else {
			out_path_direct_light += env_sample(ray_dir) * out_path_albedo;
			out_output = irradiance_correct_adv(foutput, last_alb, out_path_direct_light, alb, last_acc.x);
		}
		if (sharp_divergence(last_view_pos) > 0.0) {
			if (length(vlast_direct_light - out_direct_light) > 0.01)
				out_acc_path_pos.x = 0;
		}
	}

	if (texelFetch(depth, pos, 0).x == 0.0)
		out_output = env_sample_novoid((il.view_normal_inv * vec4(view_norm, 1.0)).xyz);
}