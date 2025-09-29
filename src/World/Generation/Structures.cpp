#include "World/World.hpp"

void world_obj::world_chunk_generator::generate_surrounding(
	const world_xzpos *curr_xz_offset,
	int32_t curr_rnd_dist
) noexcept {
	const int32_t search_dist = curr_rnd_dist + 2;
	const size_t crd_line = static_cast<size_t>((search_dist * 2) + 1);
	const size_t total_pos_checks = crd_line * crd_line;

	thread_ops::split(game.generation_thread_count, total_pos_checks, [&](int, size_t index, size_t end) {
		for (; index < end && game.is_active; ++index) {
			const int x_ind = static_cast<int>(index % crd_line) - search_dist;
			const int z_ind = static_cast<int>(index / crd_line) - search_dist;
			if (math::abs(x_ind) + math::abs(z_ind) > search_dist) continue;
			const world_xzpos full_chunk_xz_offset = { curr_xz_offset->x + x_ind, curr_xz_offset->y + z_ind };
			gen_full_chunk(&full_chunk_xz_offset);
		}
	});
}

bool world_obj::world_chunk_generator::can_del_full_chunk(
	const world_full_chunk *const full_chunk,
	const world_xzpos *const full_xz_offset,
	const world_xzpos *const thread_plr_xz_offset,
	int32_t curr_rnd_dist
) {
	const auto in_gen_dist = [&](const world_xzpos *const gen_offset) {
		return chunk_vals::offsets_dist(gen_offset, thread_plr_xz_offset) <= (curr_rnd_dist + 2);
	};
	if (in_gen_dist(full_xz_offset)) return false;
	const world_xzpos xz_global_pos = *full_xz_offset * chunk_vals::size;

	for (const world_chunk &chunk : full_chunk->subchunks) {
		for (const world_chunk::structure_info &structure : chunk.structures) {
			const world_xzpos end_pos = xz_global_pos + structure.extents.xz() + structure.start.xz();
			const world_xzpos end_offset = chunk_vals::world_to_offset(&end_pos);
			const vector2i steps = chunk_vals::step_dir_to(full_xz_offset, &end_offset);
			for (world_xzpos check_vec = *full_xz_offset; check_vec.x != end_offset.x; check_vec.x += steps.x)
			for (; check_vec.y != end_offset.y; check_vec.y += steps.y) if (in_gen_dist(&check_vec)) return false;
		}
	}

	return true;
}

// TODO: Create individual chunks instead of the entire full chunk

world_obj::world_chunk_generator::full_chunk_result world_obj::world_chunk_generator::create_full_chunk(
	const world_xzpos *const xz_offset
) {
	world_full_chunk *full_chunk = new world_full_chunk();
	noise_object::block_noise *noise_table = static_cast<noise_object::block_noise*>(
		::malloc(sizeof(noise_object::block_noise[chunk_vals::squared]))
	);
	if (!noise_table) throw std::bad_alloc();

	// Use mutex to avoid crashes/race conditions when inserting into map
	{
		std::lock_guard<std::mutex> insert_guard(reserved_access_mutex);
		reserved_map.insert({ *xz_offset, full_chunk }).first->first;
	}

	fill_noise_table(noise_table, xz_offset);
	for (int i = 0; i < chunk_vals::y_count; ++i) full_chunk->subchunks[i].construct_blocks(noise_table, i);
	return full_chunk_result{ full_chunk, noise_table };
}

world_chunk *world_obj::world_chunk_generator::create_or_get_chunk(const world_pos *const offset)
{
	world_chunk *chunk = get_chunk(offset);
	if (chunk) return chunk;
	const world_xzpos xz_offset = offset->xz();
	return create_full_chunk(&xz_offset).full_chunk->subchunks + offset->y;
}

world_chunk *world_obj::world_chunk_generator::get_chunk(const world_pos *const offset)
{
	decltype(reserved_map)::iterator found; // Use mutex in case reserved map is being edited right now
	{
		std::lock_guard<std::mutex> reserved_find_guard(reserved_access_mutex);
		found = reserved_map.find(offset->xz());
	}
	return found == reserved_map.end() ? nullptr : found->second->subchunks + offset->y;
}


block_id &world_obj::world_chunk_generator::create_block_ref(const world_pos *const position)
{
	const world_pos offset = chunk_vals::world_to_offset(position);
	world_chunk *chunk = create_or_get_chunk(&offset);
	const vector3i local_pos = chunk_vals::world_to_local(position);
	return (*(chunk->blocks ? chunk->blocks : chunk->allocate_blocks()))[local_pos.x][local_pos.y][local_pos.z];
}
void world_obj::world_chunk_generator::natural_set(
	const world_pos *const position,
	const block_properties::block_attributes *const block_properties
) {
	block_id &target = create_block_ref(position);
	target = (block_properties->strength >= block_properties::of_block(target)->strength) ?
	          block_properties->mesh_info.id :
	          target;
}


void world_obj::world_chunk_generator::fill_noise_table(
	noise_object::block_noise *const results,
	const world_xzpos *const offset
) noexcept {
	// Used per full chunk, each chunk would have the same results as they have 
	// the same XZ coordinates so no calculation is needed for each individual chunk
	constexpr double def_z_val = noise_def_vals::default_z_noise;
	constexpr double noise_step_mul = chunk_vals::noise_step / chunk_vals::size;
	const double off_x = static_cast<double>(offset->x) * chunk_vals::noise_step;
	const double off_z = static_cast<double>(offset->y) * chunk_vals::noise_step;
	const noise_obj_list *const gen = &world->world_noise_objs;

	// Loop through X and Z axis
	for (int i = 0; i < chunk_vals::squared; ++i) {
		// Get the *relative* local X and Z positions [0, 1]
		const double rel_x = ((i / chunk_vals::size) % chunk_vals::size) * noise_step_mul;
		const double rel_z = (i % chunk_vals::size) * noise_step_mul;
		// Get noise coordinates from XZ positions with given offsets
		const double pos_x = off_x + rel_x, pos_z = off_z + rel_z;

		// Store the noise results for each of the terrain noise generators
		results[i] = noise_object::block_noise((chunk_vals::surface_range * 
			gen->elevation.  octave(pos_x, def_z_val, pos_z, 3)) + chunk_vals::min_surface,
			gen->flatness.    noise(pos_x, def_z_val, pos_z),
			gen->temperature. noise(pos_x, def_z_val, pos_z),
			gen->humidity.    noise(pos_x, def_z_val, pos_z)
		);
	}
}

int_fast64_t world_obj::world_chunk_generator::noise_hash(const noise_object::block_noise *noise) noexcept
{
	return (static_cast<int_fast64_t>(
		noise->flatness * noise->height *
		noise->temperature * noise->humidity * 735498.6348f
	) << 1) ^ 8452358459;
}


void world_obj::world_chunk_generator::gen_full_chunk(const world_xzpos *const xz_offset) noexcept
{
	world_pos new_offset = { xz_offset->x, 0, xz_offset->y };
	if (world->find_full_chunk_at(xz_offset) || get_chunk(&new_offset)) return;

	world_pos struct_world_pos = { new_offset.x * chunk_vals::size, 0, new_offset.z * chunk_vals::size };
	const pos_t start_x = struct_world_pos.x, start_z = struct_world_pos.z;

	world_chunk::structure_info new_structure_info;
	const full_chunk_result full_chunk_vals = create_full_chunk(xz_offset);

	for (; new_offset.y < chunk_vals::y_count; ++new_offset.y) {
		const auto to_struct_list = [&]{
			full_chunk_vals.full_chunk->subchunks[new_offset.y].structures.emplace_back(new_structure_info);
		};

		for (int i = 0; i < chunk_vals::squared; ++i) {
			struct_world_pos.x = start_x + (i % chunk_vals::size);
			struct_world_pos.z = start_z + (i / chunk_vals::size);

			const noise_object::block_noise *const nval = full_chunk_vals.noise_table + i;
			const size_t hash = vec_hash()(struct_world_pos);
			
			struct_world_pos.y = static_cast<pos_t>(nval->height);
			
			if (struct_world_pos.y > chunk_vals::water_y &&
				!(hash % 27) &&
				create_default_tree(&new_structure_info, struct_world_pos, nval)
			) to_struct_list();
		}
	}

	::free(full_chunk_vals.noise_table);
}

// TODO: replace this mess
// how did i even come up with this?

bool world_obj::world_chunk_generator::create_default_tree(
	world_chunk::structure_info *const structure_info,
	world_pos curr_world_pos,
	const noise_object::block_noise *const noise
) noexcept {
	if (chunk_vals::is_y_outside_bounds(curr_world_pos.y)) return false;
	world_pos edit_pos = curr_world_pos;
	--edit_pos.y;
	if (!block_properties::of_block(local_get(&edit_pos))->solid) return false;
	++edit_pos.y;
	
	constexpr int min_height = 5;
	const int height = min_height + static_cast<int>(noise->temperature + noise->humidity);
	if (curr_world_pos.y > (chunk_vals::world_height - (height + 1))) return false;

	const block_properties::block_attributes *log_attribs = block_properties::of_block(block_id::log);
	const block_properties::block_attributes *leaves_attribs = block_properties::of_block(block_id::leaves);
	
	structure_info->id = structure_id::default_tree;
	structure_info->start = chunk_vals::world_to_local(&curr_world_pos) + vector3i(-2, 0, -2);
	structure_info->extents = { 4, height + 1, 4 };

	const world_pos end_pos = { curr_world_pos.x + 2, edit_pos.y + height, curr_world_pos.z + 2 };
	for (; edit_pos.y < end_pos.y; ++edit_pos.y) natural_set(&edit_pos, log_attribs);
	
	const pos_t top_y = curr_world_pos.y + 3;
	edit_pos.y -= height - (min_height - 3);
	const pos_t org_y = curr_world_pos.y + 1;

	for (edit_pos.x = curr_world_pos.x - 2; edit_pos.x <= end_pos.x; ++edit_pos.x) {
		const bool x_mid = edit_pos.x == curr_world_pos.x;
		for (edit_pos.z = curr_world_pos.z - 2; edit_pos.z <= end_pos.z; ++edit_pos.z) {
			if (x_mid && (edit_pos.z == curr_world_pos.z)) continue;
			for (edit_pos.y = org_y; edit_pos.y <= top_y; ++edit_pos.y) natural_set(&edit_pos, leaves_attribs);
		}
	}

	const auto leaf_stack = [&](const world_pos *const pos) {
		natural_set(pos, leaves_attribs);
		world_pos up_pos = *pos;
		++up_pos.y;
		natural_set(&up_pos, leaves_attribs);
	};

	curr_world_pos.y = edit_pos.y;
	for (const world_xzpos &xz_dir : chunk_vals::dirs_xz) {
		const world_pos pillar_pos = curr_world_pos + world_pos(xz_dir.x, 0, xz_dir.y);
		leaf_stack(&pillar_pos);
	}
	++curr_world_pos.y;
	natural_set(&curr_world_pos, leaves_attribs);

	return true;
}
