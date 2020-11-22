#version 460

layout(location = 0) out vec4 out_wsi;

void main(void)
{
	out_wsi = vec4(vec3(0.5), 1.0);
}