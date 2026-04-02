layout(location = 0) in uvec2 packed_pos; /* 20*3 bits XYZ region position, 4 bit world light state. */
layout(location = 1) in uint packed_tal; /* 4*3 bits RGB lighting, 16 bits texture Y, 4 bits texture X. */

uniform vec3 region_pos;
uniform float tex_y_pixel;

out vec2 tex_coords;
out vec4 light_and_fog;

void main()
{
	ivec3 ipos = ivec3(
		packed_pos.x & 0xFFFFFu,
		((packed_pos.x >> 20)) + ((packed_pos.y & 0x3FFu) << 12),
		(packed_pos.y >> 8) & 0xFFFFFu
	);
	vec3 pos = vec3(
		float(ipos.x) / 16.0,
		float(ipos.y) / 16.0,
		float(ipos.z) / 16.0
	) + region_pos;
	gl_Position = M4_origin * vec4(pos, 1.0);

	vec3 block_light = vec3(
		float((packed_tal >> 0) & 0xFu) / 15.0,
		float((packed_tal >> 4) & 0xFu) / 15.0,
		float((packed_tal >> 8) & 0xFu) / 15.0
	);
	float world_light_exposure = float(packed_pos.y >> 28) / 15.0; 
	light_and_fog = vec4(
		mix(block_light, V4_world_light.xyz, world_light_exposure),
		clamp(FLT_fog_end - length(pos.xyz) * FLT_fog_range, 0.0, 1.0)
	);

	tex_coords = vec2(
		(float((packed_tal >> 28) & 0xFu) / 15.0),
		tex_y_pixel * float((packed_tal >> 12) & 0xFFFFu)
	);
}

@

in vec2 tex_coords;
in vec4 light_and_fog;
out vec4 f;

void main()
{
	f = mix(V4_main_sky, texture(TX_blocks, tex_coords) * vec4(light_and_fog.xyz, 1.0), light_and_fog.w);
}