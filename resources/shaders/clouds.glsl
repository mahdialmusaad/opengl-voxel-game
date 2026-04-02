layout(location = 0) in vec3 base;
layout(location = 01) in vec4 inst_data;

out vec4 c;

void main()
{
	vec3 relative_pos = (vec3(inst_data.xyz + base * inst_data.w) + vec3(0.0, 0.0, FLT_gtime)) - V4_clouds_offset.xyz;
	gl_Position = M4_origin * vec4(relative_pos, 1.0);
	relative_pos.y *= 0.1;
	c = vec4(
		vec3(FLT_clouds_col),
		clamp(FLT_fog_end - length(relative_pos * 0.2) * FLT_fog_range, 0.0, 1.0)
	);
}

@

in vec4 c;
out vec4 f;

void main()
{
	f = mix(V4_main_sky, vec4(c.xyz, 1.0), c.w);
	if (c.w == 0.0) discard;
}