#version 460
#extension GL_GOOGLE_include_directive : enable

#include "0_frag.set"

layout(push_constant) uniform PushConstant {
	int albedo;
} p;

layout(location = 0) in vec3 in_n;
layout(location = 1) in vec3 in_t;
layout(location = 2) in vec3 in_b;
layout(location = 3) in vec2 in_u;

layout(location = 0) out float out_depth;
layout(location = 1) out vec4 out_albedo;
layout(location = 2) out vec4 out_normal;
layout(location = 3) out vec3 out_normal_geom;

const float height_scale = 0.15;

#include "rt.glsl"

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
	const float minLayers = 8.0;
	const float maxLayers = 32.0;
	float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));
	// number of depth layers
	//const float numLayers = 10;
	// calculate the size of each layer
	float layerDepth = 1.0 / numLayers;
	// depth of current layer
	float currentLayerDepth = 0.0;
	// the amount to shift the texture coordinates per layer (from vector P)
	vec2 P = viewDir.xy * height_scale;
	vec2 deltaTexCoords = P / numLayers;

	// get initial values
	vec2  currentTexCoords     = texCoords;
	float currentDepthMapValue = texture(samplers[p.albedo + 1], currentTexCoords).w;

	while (currentLayerDepth < currentDepthMapValue)
	{
		// shift texture coordinates along direction of P
		currentTexCoords -= deltaTexCoords;
		// get depthmap value at current texture coordinates
		currentDepthMapValue = texture(samplers[p.albedo + 1], currentTexCoords).w;
		// get depth of next layer
		currentLayerDepth += layerDepth;
	}

	// get texture coordinates before collision (reverse operations)
	vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

	// get depth after and before collision for linear interpolation
	float afterDepth  = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = texture(samplers[p.albedo + 1], prevTexCoords).w - currentLayerDepth + layerDepth;

	// interpolation of texture coordinates
	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

	return finalTexCoords;
}

void main(void)
{
	vec2 uv = vec2(in_u.x, -in_u.y) * 32.0;

	vec3 n = normalize(in_n);
	vec3 t = normalize(in_t);
	vec3 b = -normalize(in_b);

	vec3 view_norm = normalize(transpose(mat3(
		t, b, n
	)) * rt_pos_view(gl_FragCoord.xy, gl_FragCoord.z));

	uv = ParallaxMapping(uv, view_norm);

	vec4 te = texture(samplers[p.albedo], vec2(in_u.x, -in_u.y));
	if (te.w < 0.01)
		discard;
	out_depth = gl_FragCoord.z;
	out_albedo = vec4(te.xyz, 1.0);
	vec3 nmap = texture(samplers[p.albedo + 1], uv).xyz * 2.0 - 1.0;
	nmap.z *= 0.6;
	nmap = normalize(nmap);
	out_normal = vec4(t * nmap.x + b * nmap.y + n * nmap.z, 1.0);
	out_normal_geom = n;
}