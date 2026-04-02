layout(location = 0) in vec2 base_position;
layout(location = 02) in vec4 inst_dims;
layout(location = 03) in uvec2 char_and_rgba;

uniform float texture_positions[191];

out vec2 tex_coords;
out vec4 c;

void main()
{
	gl_Position = vec4(
		inst_dims.x + (base_position.x * inst_dims.z),
		inst_dims.y - (base_position.y * inst_dims.w),
		0.0, 1.0
	);
	
	tex_coords = vec2(texture_positions[char_and_rgba.x + uint(base_position.x)], base_position.y);
	c = vec4(
		(char_and_rgba.y & 0xFFu) / 255.0,
		((char_and_rgba.y >> 8) & 0xFFu) / 255.0,
		((char_and_rgba.y >> 16) & 0xFFu) / 255.0,
		((char_and_rgba.y >> 24) & 0xFFu) / 255.0
	);
}

@

out vec4 f;
in vec2 tex_coords;
in vec4 c;

void main()
{
	f = texture(TX_text, tex_coords) * c;
}