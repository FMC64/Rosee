#version 460
#extension GL_GOOGLE_include_directive : enable

struct Frame {
	mat4 mvp;
	mat3 mv_normal;
	mat3 model_world_local;
};

layout(set = 1, binding = 0) readonly buffer Dyn {
	Frame f[];
} d;

layout(location = 0) in vec3 in_p;
layout(location = 1) in vec3 in_n;

layout(location = 0) out vec3 out_n;
layout(location = 1) out vec3 out_w;

void main(void)
{
	gl_Position = d.f[gl_InstanceIndex].mvp * vec4(in_p, 1.0);
	out_n = d.f[gl_InstanceIndex].mv_normal * in_n;
	out_w = d.f[gl_InstanceIndex].model_world_local * in_p;
}
