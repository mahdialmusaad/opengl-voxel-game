const uniform vec4 inventory_texture_positions[4] = {
	vec4(0.5, 0.0, 0.2, 0.0),
	vec4(0.0, 1.0, 0.0, 0.428571),
	vec4(0.0, 1.0, 0.428571, 0.428571),
	vec4(0.0, 0.166666,0.9285714, 0.0714285714)
}; /* Data format: X, width, Y, height. */

layout(location = 0) in vec3 base;
layout(location = 01) in vec4 inst_dims; /* { X, Y, W, H }. */
layout(location = 02) in uint inst_texture_info; /* First bit -> (0 = inventory, 1 = block), rest are texture index. */

out vec2 tex_coords;
flat out uint is_block_texture;

void main()
{
	uint tex_index = inst_texture_info >> 1;
	is_block_texture = inst_texture_info & 1u;

	if (is_block_texture == 1u) {
		tex_coords = vec2(
			base.x,
			(base.z + float(tex_index)) * 16.0f
		);
	} else {
		vec4 current_texture = inventory_texture_positions[tex_index];
		tex_coords = vec2(
			(base.x * current_texture.y) + current_texture.x,
			(base.y * current_texture.w) + current_texture.z
		);
	}

	gl_Position = vec4(
		inst_dims.x + (base.x * inst_dims.z),
		inst_dims.y + (base.y * inst_dims.w),
		0.0, 1.0
	);
}

@

out vec4 f;
in vec2 tex_coords;
flat in uint is_block_texture;

void main()
{
	if (is_block_texture == 0u) {
		f = texture(TX_inventory, tex_coords);
	} else {
		f = texture(TX_blocks, tex_coords);
	}
}