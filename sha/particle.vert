#version 460

layout(location = 0) in vec2 in_pos;

void main(void)
{
	gl_Position = vec4(in_pos, 0, 1.0);
	gl_PointSize = 4.0f;
}
