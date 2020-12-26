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

layout(set = 0, binding = 1) uniform sampler2D cdepth;
layout(set = 0, binding = 2) uniform sampler2D albedo;
layout(set = 0, binding = 3) uniform sampler2D normal;

layout(location = 0) out vec3 out_output;

void main(void)
{
	out_output = vec3(0.5);
}