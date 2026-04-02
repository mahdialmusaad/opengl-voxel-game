const uniform vec3 weighted_colours[16] = {
	vec3(1.0), vec3(1.0), vec3(1.0), vec3(1.0),
	vec3(1.0), vec3(1.0), vec3(1.0), vec3(1.0),
	vec3(1.0), vec3(1.0), vec3(1.0), vec3(1.0),
	vec3(1.0, 1.0, 0.6), vec3(1.0, 1.0, 0.6),
	vec3(1.0, 0.6, 0.6), vec3(0.6, 0.6, 1.0)
};

layout(location = 0) in uvec2 int_pos;
out vec4 c;

void main()
{
	/* Y position is separated between both integers. */
	uvec3 integral_pos = uvec3(
		int_pos.x & 0x1FFFFFu,
		((int_pos.x >> 21)) + ((int_pos.y & 0x3FFu) << 11),
		int_pos.y >> 10
	);
	vec3 position = vec3(
		(float(integral_pos.x) / float(0xFFFFFu)) - 1.0,
		(float(integral_pos.y) / float(0xFFFFFu)) - (1.0 + gl_VertexID * 0.00005),
		(float(integral_pos.z) / float(0xFFFFFu)) + -1.0 + uintBitsToFloat(int_pos.x)
	);

	gl_Position = (M4_stars * vec4(position, 0.0)).xyzz;
	gl_PointSize = 1.0 + (gl_VertexID * 0.00001);
	c = vec4(weighted_colours[gl_VertexID & 0xF], FLT_stars_trnsp);
}

@

out vec4 f;
in vec4 c;

void main()
{
	f = c;
}