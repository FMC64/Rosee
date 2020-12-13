#version 460
#extension GL_GOOGLE_include_directive : enable

struct Frame {
	mat4 mvp;
	mat3 mv_normal;
};

layout(set = 1, binding = 0) readonly buffer Dyn {
	Frame f[];
} d;

layout(location = 0) in vec3 in_p;
layout(location = 1) in vec3 in_n;
layout(location = 2) in vec2 in_u;

layout(location = 0) out vec2 out_u;
layout(location = 1) out vec3 out_n;

void main(void)
{
	gl_Position = d.f[gl_InstanceIndex].mvp * vec4(in_p, 1.0);
	out_u = in_u;
	out_n = d.f[gl_InstanceIndex].mv_normal * in_n;
}
