#include "world/generate.h"
#include "world/locate.h"
#include "world/modify.h"

#include "directives/dcast.h"
#include "directives/dfree.h"

#include "player/movement.h"

#include "values/elements.h"
#include "values/state.h"

#if VX_WLD_DEBUG - 0
#include "graphics/glfw.h"
#endif

#include "utils/noise.h"

#include <stdlib.h>

static int vxwld_force_set(const wpos *pos, vxblk id)
{
	if (vxelm_elements[vxwld_get(pos)].strength > vxelm_elements[id].strength) return 0;

	wpos global_offset, region_offset;
	vxwld_global_position_offset(pos, &global_offset);
	vxwld_regoff_from_globoff(&global_offset, &region_offset);

	vxwld_region *region = vxwld_create_region(&region_offset);
	if (!region) return 0;

	return vxwld_set(pos, id);
}

static intmax_t vxwld_pos_hash(const wpos *pos)
{
	return pos->x ^ (((pos->y) ^ (pos->z >> 1)) << 1);
}

static int vxwld_attempt_tree(wpos *base_pos)
{
	if (vxwld_get(base_pos)) return 0;
	if (base_pos->y <= (VX_WLD_WATER_LEVEL + 2)) return 0;

	const double tree_noise = vxns_noise_2d(
		vxwld_noise,
		VX_WLD_NOISE_TRAVEL * VX_CAST(double, base_pos->x),
		VX_WLD_NOISE_TRAVEL * VX_CAST(double, base_pos->z)
	);

	if ((*VX_CAST(const pos_type *, &tree_noise) - vxwld_pos_hash(base_pos)) % (63 + (783 * (tree_noise > 0.2 || tree_noise < -0.1)))) return 0;
	
	for (int i = 0; i < 6; ++i) {
		const wpos log_pos = { base_pos->x, base_pos->y + i, base_pos->z }; 
		vxwld_force_set(&log_pos, 6);
	}

	static const wpos leaves[] = {
		{ -2, 0, -2 }, { -1, 0, -2 }, { 0, 0, -2 }, { 1, 0, -2 }, { 2, 0, -2 },
		{ -2, 0, -1 }, { -1, 0, -1 }, { 0, 0, -1 }, { 1, 0, -1 }, { 2, 0, -1 },
		{ -2, 0,  0 }, { -1, 0,  0 }, { 1, 0,  0 }, { 2, 0,  0 },
		{ -2, 0,  1 }, { -1, 0,  1 }, { 0, 0,  1 }, { 1, 0,  1 }, { 2, 0,  1 },
		{ -2, 0,  2 }, { -1, 0,  2 }, { 0, 0,  2 }, { 1, 0,  2 },

		{ -2, 1, -2 }, { -1, 1, -2 }, { 0, 1, -2 }, { 1, 1, -2 }, { 2, 1, -2 },
		{ -2, 1, -1 }, { -1, 1, -1 }, { 0, 1, -1 }, { 1, 1, -1 }, { 2, 1, -1 },
		{ -2, 1,  0 }, { -1, 1,  0 }, { 1, 1,  0 }, { 2, 1,  0 },
		{ -2, 1,  1 }, { -1, 1,  1 }, { 0, 1,  1 }, { 1, 1,  1 }, { 2, 1,  1 },
		{ -2, 1,  2 }, { -1, 1,  2 }, { 0, 1,  2 }, { 1, 1,  2 }, { 2, 1,  2 },

		{ -1, 2, -2 }, {  0, 2, -2 }, { 1, 2, -2 }, { 2, 2, -2 },
		{ -2, 2, -1 }, { -1, 2, -1 }, { 0, 2, -1 }, { 1, 2, -1 }, { 2, 2, -1 },
		{ -2, 2,  0 }, { -1, 2,  0 }, { 1, 2,  0 }, { 2, 2,  0 },
		{ -2, 2,  1 }, { -1, 2,  1 }, { 0, 2,  1 }, { 1, 2,  1 }, { 2, 2,  1 },
		{ -2, 2,  2 }, { -1, 2,  2 }, { 0, 2,  2 }, { 1, 2,  2 }, { 2, 2,  2 },

		{ -1, 3, 0 }, { 0, 3, -1 }, { 1, 3, 0 }, { 0, 3, 1 },
		{ -1, 4, 0 }, { 0, 4, -1 }, { 0, 4, 0 }, { 1, 4, 0 }, { 0, 4, 1 },
	};

	for (size_t i = 0; i < sizeof leaves / sizeof *leaves; ++i) {
		const wpos *leaf = leaves + i;
		wpos leaf_pos;
		wpos_add(&leaf_pos, leaf, base_pos);
		leaf_pos.y += 2;
		vxwld_force_set(&leaf_pos, 7);
	}

	return 1;
}

/* Decorate the given chunk. */
static int vxwld_decor_chunk(vxwld_chunk *chunk)
{
	if (!chunk->blocks.data && !(chunk->blocks.data = VX_CAST(vxblk *, calloc(VX_WLD_CHUNK_BLOCKS, sizeof *chunk->blocks.data)))) return 0;
	chunk->is_decorated = 1u;

	wpos chunk_global_pos;
	vxwld_chunk_globpos(chunk, &chunk_global_pos);
	intmax_t air_count = 0;

	pos_type grass_y[VX_WLD_CHUNK_XBLKS][VX_WLD_CHUNK_ZBLKS];
	int any_struct_added = 0;

	for (int i = 0; i < VX_WLD_CHUNK_XBLKS * VX_WLD_CHUNK_ZBLKS; ++i) {
		const pos_type x_ind = i / VX_WLD_CHUNK_ZBLKS, z_ind = i % VX_WLD_CHUNK_ZBLKS;
		const pos_type xz_index = VX_INDEX_FROM_XYZ(x_ind, 0, z_ind, VX_WLD_CHUNK_YBLKS, VX_WLD_CHUNK_ZBLKS);

		const double noise_height = vxns_fractal_2d(
			vxwld_noise, 4,
			VX_CAST(double, chunk_global_pos.x + x_ind) * VX_WLD_NOISE_TRAVEL,
			VX_CAST(double, chunk_global_pos.z + z_ind) * VX_WLD_NOISE_TRAVEL
		) * VX_WLD_NOISE_MULT;

		const pos_type noise_y = VX_CAST(pos_type, noise_height);
		grass_y[x_ind][z_ind] = noise_y;
		
		pos_type y_count = (noise_y - chunk_global_pos.y) + 1;
		if (y_count > VX_WLD_CHUNK_YBLKS) { grass_y[x_ind][z_ind] = -1; y_count = VX_WLD_CHUNK_YBLKS; }
		else if (y_count < 0) { grass_y[x_ind][z_ind] = -1; y_count = 0; }
		else grass_y[x_ind][z_ind] = y_count;

		for (pos_type y = 0; y < VX_WLD_CHUNK_YBLKS; ++y) {
			vxblk *existing = chunk->blocks.data + (xz_index + (y * VX_WLD_CHUNK_YBLKS));
			if (*existing != 0) continue;

			const pos_type global_y = chunk_global_pos.y + y;
			const pos_type difference = global_y - noise_y;

			const int is_suspended = difference > 0;
			const int is_surface = difference == 0;
			const int is_underground = difference < 0;

			const pos_type underground_intensity = noise_y - global_y;
			vxblk toplace;

			enum { air, grass, dirt, stone, sand, water };

			if (is_underground) {
				if (underground_intensity > 4) toplace = stone;
				else toplace = dirt;
			} else if (is_suspended) {
				if (global_y - VX_WLD_WATER_LEVEL < 1) toplace = water;
				else toplace = air;
			} else if (is_surface) {
				if (global_y - VX_WLD_WATER_LEVEL <= 1) toplace = sand;
				else toplace = grass;
			} else toplace = air;
			
			*existing = toplace;
			air_count += toplace == 0;
		}
	}

	for (int i = 0; i < VX_WLD_CHUNK_XBLKS * VX_WLD_CHUNK_ZBLKS; ++i) {
		const pos_type x_ind = i / VX_WLD_CHUNK_ZBLKS, z_ind = i % VX_WLD_CHUNK_ZBLKS;
		pos_type cur_grass_y = grass_y[x_ind][z_ind];
		if (cur_grass_y == -1) continue;

		wpos global_pos = {
			chunk_global_pos.x + x_ind,
			chunk_global_pos.y + cur_grass_y,
			chunk_global_pos.z + z_ind
		};
		any_struct_added |= vxwld_attempt_tree(&global_pos);
	}

	if (!any_struct_added && air_count >= VX_WLD_CHUNK_BLOCKS) VX_FREE(chunk->blocks.data);
	return 1;
}


int vxwld_offset_close_enough(const wpos *global_offset)
{
	const double allowed_distance = (VX_CAST(double, vxwld_info.render_distance) * (VX_CAST(double, VX_WLD_CHUNK_XBLKS) * 0.5)) + (VX_CAST(double, VX_WLD_CHUNK_XBLKS) * 0.5);
	const dvec3 dist_vec = {
		fabs(vxplr_inst.pos.x - (VX_CAST(double, (global_offset->x * VX_WLD_CHUNK_XBLKS)) + (VX_WLD_CHUNK_XBLKS * 0.5))),
		fabs(vxplr_inst.pos.y - (VX_CAST(double, (global_offset->y * VX_WLD_CHUNK_YBLKS)) + (VX_WLD_CHUNK_YBLKS * 0.5))),
		fabs(vxplr_inst.pos.z - (VX_CAST(double, (global_offset->z * VX_WLD_CHUNK_ZBLKS)) + (VX_WLD_CHUNK_ZBLKS * 0.5)))
	};
	return dvec3_length(&dist_vec) < allowed_distance;
}


size_t vxwld_generate_around(VX_NO_ARG)
{
	if (!vxtg_toggles.generation_active) return 0u;
#if VX_WLD_DEBUG - 0
	const double generation_start = glfwGetTime();
#endif

	const pos_type rd_axis = (vxwld_info.render_distance * 2) + 1;
	size_t num_created = 0;

	/* Loop through all offsets in a cube around the player. */
	for (pos_type i = 0, rd_count = rd_axis * rd_axis * rd_axis; i < rd_count; ++i) {
		const wpos global_offset = {
			vxplr_inst.offset.x + (-vxwld_info.render_distance + VX_XPOS_FROM_INDEX(i, rd_axis, rd_axis)),
			vxplr_inst.offset.y + (-vxwld_info.render_distance + VX_YPOS_FROM_INDEX(i, rd_axis, rd_axis)),
			vxplr_inst.offset.z + (-vxwld_info.render_distance + VX_ZPOS_FROM_INDEX(i, rd_axis))
		};

		/* Do not create if the chunk is too far or not in camera view. */
		if (!vxwld_offset_close_enough(&global_offset)) continue;

		/* Create region if the given offset is outside any existing ones. */
		wpos region_offset;
		vxwld_regoff_from_globoff(&global_offset, &region_offset);
		vxwld_region *region = vxwld_create_region(&region_offset);
		if (!region) continue;

		/* Do not generate if the chunk has already been generated. */
		vxwld_chunk *chunk = vxwld_chunk_from_regoff(region, &global_offset);
		if (chunk->is_decorated) continue;

		vxwld_decor_chunk(chunk);
		vxwld_queue_change(chunk);

		/* Other chunks bordering this one also need to be updated. */
		for (int s = 0; s < 27; ++s) {
			if (s == 9 + 3 + 1) continue; /* Skip current chunk. */
			const wpos surrounding_offset = {
				(global_offset.x - 1) + (s / 9),
				(global_offset.y - 1) + ((s / 3) % 3),
				(global_offset.z - 1) + (s % 3)
			};

			vxwld_chunk *surrounding_chunk = vxwld_chunk_from_offset(&surrounding_offset);
			if (!surrounding_chunk || !surrounding_chunk->blocks.data) continue;
			vxwld_queue_change(surrounding_chunk);
		}

		if (++num_created >= VX_WLD_MAX_GENERATE) break;
	}

#if VX_WLD_DEBUG - 0
	if (num_created) {
		const double generation_elapsed = (glfwGetTime() - generation_start) * 1000.0;
		printf("[ GENERATION ] Generated %zu chunks (%.3fms/chunk, %.3fms total)\n",
			num_created, generation_elapsed / VX_CAST(double, num_created), generation_elapsed
		);
	}
#endif

	return num_created;
}
