layout(set = 0, binding = 2) uniform sampler2D cdepth;
layout(set = 0, binding = 3) uniform sampler2D albedo;
layout(set = 0, binding = 4) uniform sampler2D normal;

struct Probe {
	vec3 pos;
	float depth;
};

layout(constant_id = 2) const uint probe_layer_count = 1;

layout(set = 0, binding = 5) buffer Probes {
	Probe probes[];
} probes_pos;
