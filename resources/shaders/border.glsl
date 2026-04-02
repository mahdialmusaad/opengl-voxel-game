layout(location = 0) in vec3 base_pos;
layout(location = 01) in vec3 inst_rgba;
layout(location = 02) in vec4 inst_scale;

out vec4 c;

void main()
{
	gl_Position = M4_origin * vec4((base_pos * inst_scale.xyz) - mix(V4_chunk_lpos.xyz, V4_region_lpos.xyz, inst_scale.w), 1.0);
	c = vec4(inst_rgba, 1.0);
}

@

in vec4 c;
out vec4 f;

void main()
{
	f = c;
}