#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

struct Frame {
	mat4 mvp;
};

layout(set = 1, binding = 0) readonly buffer Dyn {
	Frame f[];
} d;

layout(location = 0) in vec3 in_p;
layout(location = 1) in vec3 in_n;
layout(location = 2) in vec2 in_u;

layout(location = 0) out vec2 out_u;

void main(void)
{
	gl_Position = d.f[gl_InstanceIndex].mvp * vec4(in_p, 1.0);
	out_u = in_u;
}
