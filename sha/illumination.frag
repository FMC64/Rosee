#version 460
#extension GL_GOOGLE_include_directive : enable

layout(constant_id = 0) const int sample_count = 1;
layout(constant_id = 1) const float sample_factor = 1.0;

layout(set = 0, binding = 0) uniform Illum {
	mat4 cam_proj;
	vec3 sun;
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

layout(location = 0) out vec3 out_output;

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

vec3 rt_current_view(void)
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
	/*if (origin.z < camera.near || origin.z > camera.far) {
		if (dir.z > 0.0)
			len = (camera.near - origin.z) / dir.z;
		else
			len = (camera.far - origin.z) / dir.z;
		origin += dir * len;
	}*/
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

// returns t intersection
float rt_cell_end(vec2 p, vec2 d, float size)
{
	vec2 cell_tl = floor(p / size) * size;
	return rt_inter_rect(cell_tl, cell_tl + size, p, d);
}

bool rt_traceRay(vec3 origin, vec3 dir, out vec2 pos)
{
	vec3 p0, p1;
	rt_project_ray(origin, dir, p0, p1);

	vec3 normal = normalize(texelFetch(normal, ivec2(p0.xy), 0)).xyz;

	float bias = 0.025 * (1.0 - max(dot(normalize(origin), -normal), 0.0));

	vec2 dir2 = p1.xy - p0.xy;
	if (dir2 == vec2(0.0))
		return false;
	vec2 dir2n = -dir2;
	float dir_len = 1.0 / length(dir2);
	float dir_pp_bias = dir_len * (1.0 / 64.0);
	//vec2 dir2_norm = dir2 * dir_len;

	float t = 0.0;

	int level = 0;

	float t_max;
	if (!rt_inter_rect_strong(vec2(0.0), vec2(textureSize(albedo) - 1), p0.xy, dir2, dir_pp_bias, t_max))
		return false;

	vec3 p = p0;
	uint it = 0;

	if (dot(normal, dir) < 0.0) {
		pos = p.xy;
		return true;
	}

	const int trace_res = 0;
	t += rt_cell_end(p.xy, dir2, float(1 << trace_res)) + dir_pp_bias;
	p = mix(p0, p1, t);

	//uint max_it = 512;//rt_fb.depth_buffer_max_it >> max_it_fac;
	for (int i = 0; i < 512; i++) {
		float d = textureLod(depth, p.xy * il.depth_size, level).x;
		if (level == 0)
			d += rt_z_to_depth(rt_depth_to_z(d) + bias);
		if (p.z <= d) {
			if (level == 0) {
				pos = p.xy;
				return true;
			}
			level--;
		} else {
			t += dir_len * float(1 << (level + trace_res));
			if (t >= t_max)
				return false;
			p = mix(p0, p1, t);
			level++;
		}
	}
	return false;
}

void main(void)
{
	ivec2 pos = ivec2(gl_FragCoord.xy);
	out_output = vec3(0.0);
	int sample_done[sample_count];
	//int its = 0;
	for (int i = 0; i < sample_count; i++)
		sample_done[i] = 0;
	int i = 0;
	for (int k = 0; k < sample_count; k++) {
		float d = texelFetch(cdepth, pos, i).x;
		vec3 alb = texelFetch(albedo, pos, i).xyz;
		vec3 norm = normalize(texelFetch(normal, pos, i).xyz);
		float align = dot(norm, il.sun);

		vec2 rt_pos;
		if (rt_traceRay(rt_current_view(), il.sun, rt_pos))
			align = 0.0;

		float illum = max(align, 0.05);
		vec3 outp = alb * illum * 2.5;

		float count = 0;
		for (int j = 0; j < sample_count; j++) {
			bool same = texelFetch(cdepth, pos, j).x == d;
			sample_done[j] += same ? 1 : 0;
			count += same ? 1.0 : 0.0;
		}
		out_output += outp * count;
		//its++;

		for (int j = 0; j < sample_count; j++) {
			if (sample_done[i] == 0)
				break;
			i++;
			if (i >= sample_count) {
				out_output *= sample_factor;
				//out_output = vec3(float(its) / float(sample_count));
				//out_output = vec3(texture(depth, gl_FragCoord.xy * il.depth_size, 6).x);
				return;
			}
		}
	}
}