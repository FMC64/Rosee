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
layout(set = 0, binding = 2) uniform sampler2DMS albedo;
layout(set = 0, binding = 3) uniform sampler2DMS normal;

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

void main(void)
{
	ivec2 pos = ivec2(gl_FragCoord.xy);
	vec3 view = rt_current_view();
	vec3 view_norm = normalize(view);

	vec3 alb = texelFetch(albedo, pos, 0).xyz;
	vec3 norm = texelFetch(normal, pos, 0).xyz;
	float align = dot(norm, il.sun);
	vec3 direct_light = alb * max(0.0, align) * 2.5;
	direct_light += alb * env_sample((il.view_normal_inv * vec4(norm, 1.0)).xyz);

	out_output = direct_light;
	if (texelFetch(cdepth, pos, 0).x == 0.0)
		out_output = env_sample_novoid((il.view_normal_inv * vec4(view_norm, 1.0)).xyz);
}