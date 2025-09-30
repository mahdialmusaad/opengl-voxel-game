#include "World.hpp"

world_obj::world_obj(world_acc_player *player) noexcept :
    world_noise_objs({1337, 1338, 1339, 1340, 1341}),
    world_plr(player)
{
	m_generator.world = this; // Set world pointer for nested generator struct

	// VAO and VBO for debug chunk borders
	m_borders_vao = ogl::new_vao();
	glEnableVertexAttribArray(0);

	// Coordinates of cube edges
	const float cszf = static_cast<float>(chunk_vals::size);
	const float border_cube_line_pos[] = {
		0.0f, 0.0f, 0.0f, // Bottom left back
		cszf, 0.0f, 0.0f, // Bottom right back
		0.0f, 0.0f, cszf, // Bottom left front
		cszf, 0.0f, cszf, // Bottom right front
		0.0f, cszf, 0.0f, // Top left back
		cszf, cszf, 0.0f, // Top right back
		0.0f, cszf, cszf, // Top left front
		cszf, cszf, cszf, // Top right front
	};

	m_borders_vbo = ogl::new_buf(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof border_cube_line_pos, border_cube_line_pos, 0);

	// Borders outline line indexes (each pair is a line's start+end vertex index from the above array)
	const uint8_t border_cube_line_inds[] = {
		0, 1,  0, 2,  1, 3,  2, 3,
		0, 4,  1, 5,  2, 6,  3, 7,
		4, 5,  4, 6,  5, 7,  6, 7
	};
	m_borders_ebo = ogl::new_buf(GL_ELEMENT_ARRAY_BUFFER);
	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof border_cube_line_inds, border_cube_line_inds, 0);

	// Create VAO for the world buffers to store buffer settings
	m_world_vao = ogl::new_vao();
	glEnableVertexAttribArray(0);
	glVertexAttribDivisor(0, 1);

	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	// Very important definition of a plane/quad for block rendering
	// Laid out like this to allow for swizzling (e.g. vector.xyz) which is "essentially free" in shaders
	// See https://www.khronos.org/opengl/wiki/GLSL_Optimizations for more information

	const float plane_verts[] = {
	//              vec4 0                    vec4 1                    vec4 2
		1.0f, 1.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f, 0.0f,  // Vertex 1
		1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f, 0.0f,  // Vertex 2
		0.0f, 1.0f, 0.0f, 1.0f,   1.0f, 1.0f, 0.0f, 1.0f,   1.0f, 0.0f, 0.0f, 0.0f,  // Vertex 3
		0.0f, 0.0f, 0.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,   1.0f, 1.0f, 0.0f, 0.0f,  // Vertex 4
	//        x     z     0     1       y     z     0     1       y     w     0     0
	};

	// Instanced VBO that contains plane data, 12 floats per vertex
	m_world_plane_vbo = ogl::new_buf(GL_ARRAY_BUFFER);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(float[12]), nullptr);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(float[12]), reinterpret_cast<const void*>(sizeof(float[4])));
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(float[12]), reinterpret_cast<const void*>(sizeof(float[8])));
	glBufferStorage(GL_ARRAY_BUFFER, sizeof plane_verts, plane_verts, 0);

	// World data buffer for instanced face data
	m_world_inst_vbo = ogl::new_buf(GL_ARRAY_BUFFER);
	glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 0, nullptr);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW); // Buffer to allow mapping (size 0 is invalid)

	// Shader storage buffer object (SSBO) to store chunk positions and face indexes for each chunk face (location = 0)
	m_world_ssbo = ogl::new_buf(GL_SHADER_STORAGE_BUFFER);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_world_ssbo);
	m_world_dib = ogl::new_buf(GL_DRAW_INDIRECT_BUFFER); // Buffer for indirect draw commands (4 GLuints per command)

	update_render_distance(m_render_distance); // Initial update and buffer sizing

	// Chunk creation loop on a separate thread to avoid blocking main game loop
	if (!game.DB_exit_on_loaded) m_generation_thread = std::thread(&world_obj::generation_loop, this, false);
}

void world_obj::draw_entire_world() noexcept
{
	game.shaders.programs.blocks.bind_and_use(m_world_vao); // Use correct VAO and shader program
	if (m_do_buffers_update) update_inst_buffer_data(); // Update buffers if needed

	// Draw the entire world in a single draw call using the indirect buffer, essentially
	// doing an instanced draw call (glDrawArraysInstancedBaseInstance) for each 'chunk face'.
	// See https://registry.khronos.org/OpenGL-Refpages/gl4/html/glMultiDrawArraysIndirect.xhtml for more information.

	glMultiDrawArraysIndirect(GL_TRIANGLE_STRIP, nullptr, m_indirect_calls, 0);
}

void world_obj::draw_enabled_borders() noexcept
{
	if (!game.display_chunk_borders) return; // Only render if enabled
	game.shaders.programs.border.bind_and_use(m_borders_vao); // Switch to borders VAO and shader
	glDepthFunc(GL_ALWAYS); // Always draw on top
	glDrawElements(GL_LINES, 24, GL_UNSIGNED_BYTE, nullptr); // Draw using indexed line data
	glDepthFunc(GL_LEQUAL); // Default render depth function
}

world_chunk *world_obj::world_pos_to_chunk(const world_pos *pos) const noexcept
{
	const world_pos chunk_offset = chunk_vals::world_to_offset(pos); // Get offset of position
	return find_chunk_at(&chunk_offset); // Find chunk at the calculated offset
}

block_id world_obj::block_at(const world_pos *pos) const noexcept
{
	world_chunk *const chunk = world_pos_to_chunk(pos); // Get the chunk that contains the given position
	if (chunk && chunk->blocks) {
		const vector3i in_pos = chunk_vals::world_to_local(pos);
		return (*chunk->blocks)[in_pos.x][in_pos.y][in_pos.z]; // Get block at the local pos in the chunk
	} else return block_id::air; // If no chunk is found, assume air
}

void world_obj::set_block_at_to(const world_pos *pos, block_id block) noexcept
{
	if (chunk_vals::is_y_outside_bounds(pos->y)) return; // Validate position

	// Get the full chunk that contains the given position
	const world_pos offset = chunk_vals::world_to_offset(pos);
	const world_xzpos xz_offset = offset.xz();
	world_full_chunk *full_chunk = find_full_chunk_at(&xz_offset);
	if (!full_chunk) return; // Ignore blocks outside render distance or in invalid chunks

	world_chunk *chunk = full_chunk->subchunks + offset.y; // Get the inner chunk
	const vector3i in_pos = chunk_vals::world_to_local(pos); // Get local chunk position of the block

	// Check if the inner chunk has a null block array
	if (!chunk->blocks) {
		if (block == block_id::air) return; // Ignore uneccessary changes (air to empty chunk)
		else chunk->allocate_blocks(); // Use normal block storage
	}

	(*(chunk->blocks))[in_pos.x][in_pos.y][in_pos.z] = block; // Change block at local position
	quad_data_t *const mesh_data = update_chunk_immediate(full_chunk, chunk, &xz_offset, offset.y, nullptr);

	// Update bordering chunks if changed block was on the chunk's corner
	nearby_data_obj nearby_data[6];
	const int count = fill_nearby_data(&offset, nearby_data, true);
	for (int i = 0; i < count; ++i) {
		const nearby_data_obj *nearby = nearby_data + i;
		if (!chunk_vals::is_bordering(in_pos, nearby->dir_index)) continue;
		const world_xzpos nearby_xz_offset = nearby->offset.xz();
		update_chunk_immediate(nearby->full_chunk, nearby->chunk, &nearby_xz_offset, nearby->offset.y, mesh_data);
	}

	delete[] mesh_data;
}

quad_data_t *world_obj::update_chunk_immediate(
	const world_full_chunk *const full_chunk,
	world_chunk *const updating_chunk,
	const world_xzpos *const xz_offset,
	pos_t y_offset,
	quad_data_t *mesh_data_array
) {
	if (!mesh_data_array) mesh_data_array = new quad_data_t[chunk_vals::total_faces];

	updating_chunk->mesh_faces(
		rendered_map,
		full_chunk,
		xz_offset,
		updating_chunk->face_counters,
		y_offset,
		mesh_data_array
	);
	m_do_buffers_update = true;
	return mesh_data_array;
}

uintmax_t world_obj::fill_blocks(world_pos from, world_pos to, block_id b_id) noexcept
{
	// Force valid position - ensure Y position is in range
	from.y = math::clamp(from.y, pos_t{}, static_cast<pos_t>(chunk_vals::world_height));
	to.y = math::clamp(to.y, pos_t{}, static_cast<pos_t>(chunk_vals::world_height));

	// Determine direction to move in each axis
	const world_pos step = chunk_vals::step_dir_to(&from, &to);
	
	// Deny any extremely large changes
	const uintmax_t total_sets = static_cast<uintmax_t>(
		math::abs(to.x - from.x) *
		math::abs(to.y - from.y) *
		math::abs(to.z - from.z)
	);
	if (total_sets > 0xFFFF) return total_sets;

	// Ensure a block is still placed if any to-from values are equal
	for (int i = 0; i < 3; ++i) {
		pos_t &to_axis = to[i];
		if (to_axis == from[i]) to_axis += step[i];
	}

	// Set all of the valid blocks from fx, fy, fz to tx, ty, tz (inclusive) as the given block ID
	// TODO: Mesh queue
	world_pos set{};
	for (set.x = from.x; set.x != to.x; set.x += step.x) {
		for (set.y = from.y; set.y != to.y; set.y += step.y) {
			for (set.z = from.z; set.z != to.z; set.z += step.z) set_block_at_to(&set, b_id);
		}
	}

	return 0; // Signal success
}

world_chunk *world_obj::find_chunk_at(const world_pos *chunk_offset) const noexcept
{
	// Check if the given Y offset is in range
	if (chunk_offset->y >= chunk_vals::y_count || chunk_offset->y < 0) return nullptr;
	const world_xzpos xz_offset = chunk_offset->xz();
	world_full_chunk *full_chunk = find_full_chunk_at(&xz_offset);  // Find full chunk at given XZ offset
	return full_chunk ? full_chunk->subchunks + chunk_offset->y : nullptr; // Return chunk if the full chunk exists
}

world_full_chunk *world_obj::find_full_chunk_at(const world_xzpos *offset) const noexcept
{
	const auto it = rendered_map.find(*offset); // Find full chunk at given XZ offset
	return it != rendered_map.end() ? it->second : nullptr; // Return full chunk if it was found
}

pos_t world_obj::highest_solid_y_at(world_xzpos *xz_position) const noexcept
{
	// Get chunk offsets containing the given XZ position
	const world_xzpos xz_offset = chunk_vals::world_to_offset(xz_position);
	const world_full_chunk *const containing_full_chunk = find_full_chunk_at(&xz_offset);

	// Get the local chunk position (pos. of block inside a chunk) from the world position
	vector3i chunk_pos = {
		chunk_vals::world_to_local_one(xz_position->x), 0,
		chunk_vals::world_to_local_one(xz_position->y)
	};

	for (int y = chunk_vals::y_count - 1; y >= 0; --y) {
		const world_chunk *const chunk = containing_full_chunk->subchunks + y;
		if (!chunk || !chunk->blocks) continue; // Check if it is a valid chunk with blocks
		
		// Search the local XZ coordinate inside found chunk from top to bottom
		const pos_t world_y_pos = y * chunk_vals::size;
		for (chunk_pos.y = chunk_vals::less; chunk_pos.y >= 0; --chunk_pos.y) {
			if ((*(chunk->blocks))[chunk_pos.x][chunk_pos.y][chunk_pos.z] != block_id::air)
				return world_y_pos + static_cast<pos_t>(chunk_pos.y);
		}
	}

	// Fallback to bottom position
	return 0;
}

bool world_obj::xz_in_rnd_dist(const world_xzpos *chunk_offset) const noexcept
{
	// Check if the XZ offset between the given and player together are less than the render distance
	const world_xzpos plr_xz_offset = world_plr->offset.xz();
	return chunk_vals::offsets_dist(chunk_offset, &plr_xz_offset) <= static_cast<pos_t>(m_render_distance);
}

void world_obj::update_render_distance(int32_t new_rnd_dist) noexcept
{
	// Render distance determines how many chunks in a 'star' pattern will be generated
	// from the initial player chunk (e.g. a render distance of 0 has only the player's chunk,
	// whereas a render distance of 1 has 4 extra chunks surrounding it = 5 in total).

	// Ensure render distance is within bounds, otherwise still allow if it is increasing (for debug purposes)
	if (game.is_loop_active && (
		new_rnd_dist < val_limits::render_limits.min ?
		new_rnd_dist <= m_render_distance :
		false)
	) return;
	m_render_distance = new_rnd_dist;

	// Signal generation thread to run if it is waiting for an event
	if (game.is_loop_active) {
		signal_generation_thread();
		formatter::log(formatter::fmt("Render distance changed (%d)", m_render_distance));
	}

	m_do_arrays_update = true; // Update indirect, translucent and ssbo arrays
}

void world_obj::generation_loop(bool ismain) noexcept
{
	typedef world_chunk::world_map::value_type w_val_t;
	struct meshing_data {
		world_xzpos full_offset;
		world_full_chunk *full_chunk;
		world_chunk::face_counts_obj meshed_counters[chunk_vals::y_count][6];
	};

	static std::vector<meshing_data> to_mesh;
	static std::vector<decltype(to_mesh)::iterator> remove_mesh_state;

	if (!ismain) goto generation_thread_entry; // Thread section

	switch (m_gen_thread_state)
	{
	case gen_state_en::both_finish: // No work currently, main and generation finished
		if (m_do_gen_update) goto gen_update; // Start thread and set state to active if an update is requested
		break;
	case gen_state_en::await_confirm: // Commit given chunks from generation thread for meshing
		// Remove chunks outside render distance from main map as well as their mesh data
		for (auto it = rendered_map.begin(); it != rendered_map.end();) {
			if (xz_in_rnd_dist(&it->first)) { ++it; continue; }
			for (int y = 0; y < chunk_vals::y_count; ++y) {
				world_chunk *const chunk = it->second->subchunks + y;
				::memset(chunk->quads_ptr, 0, sizeof chunk->quads_ptr);
				chunk->state = 0u;
			}

			m_generator.reserved_map.insert(*it);
			it = rendered_map.erase(it);
		}

		// Add any chunks from the generation map that entered render distance to the render map
		for (auto it = to_mesh.begin(); it != to_mesh.end();) {
			if (!xz_in_rnd_dist(&it->full_offset)) { ++it; continue; }
			if (!find_full_chunk_at(&it->full_offset)) rendered_map.insert(std::make_pair(it->full_offset, it->full_chunk ));
			remove_mesh_state.emplace_back(it); // Save iterator
			++it;
		}

		m_gen_thread_state = gen_state_en::active; // Set active state for thread conditional
		m_gen_conditional.notify_one(); // Notify generation thread so it can begin generating
		m_do_buffers_update = true; // Update which chunks can be rendered
		break;
	case gen_state_en::gen_finished: // Generation thread finished
		// Remove 'meshing' state from affected chunks
		for (auto &it : remove_mesh_state) it->full_chunk->full_state &= ~world_full_chunk::state_en::generation_mark;

		// Copy thread face counters result into chunks to use new data
		for (auto &it : to_mesh) {
			for (int i = 0; i < chunk_vals::y_count; ++i) {
				::memcpy(it.full_chunk->subchunks[i].face_counters, it.meshed_counters[i], sizeof *it.meshed_counters);
				it.full_chunk->subchunks[i].state &= ~world_chunk::states_en::use_tmp_data;
			}
		}

		to_mesh.clear();

		remove_mesh_state.clear();
		m_do_buffers_update = true; // Update world buffers on next draw
		// Wait until the next 'generate' signal unless one was given whilst the thread was active
		if (m_do_gen_update) {
		gen_update:
			m_gen_thread_state = gen_state_en::active;
			m_gen_conditional.notify_one();
			m_do_gen_update = false;
		} else m_gen_thread_state = gen_state_en::both_finish;
		break;
	default:
		break;
	}

	return;
generation_thread_entry:
	to_mesh.clear(); // Clear for multiple world entries
	game.generation_thread_count = math::max(1, game.available_threads - 1);
	nearby_full_data_obj nb_full_data[4];

do { // I'd prefer if I didn't have to indent the whole generation logic for this
	const world_xzpos curr_plr_xz_offset = world_plr->offset.xz(); // Generator-local player offset
	const int32_t curr_rnd_dist = m_render_distance; // Generator-local render distance

	// Calculate all surrounding chunks as well as those further than the render distance to determine structure placement
	m_generator.generate_surrounding(&curr_plr_xz_offset, curr_rnd_dist);
	if (!game.is_active) return;

	// Determine chunks that need deletion and add those that are going to become visible
	// and the ones near the visible ones to the meshing list
	for (auto it = m_generator.reserved_map.begin(); it != m_generator.reserved_map.end();) {
		const pos_t total_offset_dist = chunk_vals::offsets_dist(&it->first, &curr_plr_xz_offset);
		if (total_offset_dist > curr_rnd_dist) {
			const bool is_deleting = m_generator.can_del_full_chunk(it->second, &it->first, &curr_plr_xz_offset, curr_rnd_dist);
			if (is_deleting) { delete it->second; it = m_generator.reserved_map.erase(it); } else ++it;
			continue;
		}

		to_mesh.emplace_back(meshing_data{ it->first, it->second, {} }); // Add current full chunk (in render distance) to meshing list
		it->second->full_state |= world_full_chunk::state_en::generation_mark; // Mark as being modified

		// Do the same for those adjacent to the current full chunk (that hasnt been added already)
		const nearby_full_data_obj *const nb_end_ptr = nb_full_data + fill_nearby_full_data(&it->first, nb_full_data);
		for (const nearby_full_data_obj *curr_nb = nb_full_data; curr_nb != nb_end_ptr; ++curr_nb) {
			if ((curr_nb->full_chunk->full_state & world_full_chunk::state_en::generation_mark) ||
			    chunk_vals::offsets_dist(&it->first, &curr_nb->xz_offset) > curr_rnd_dist) continue;
			curr_nb->full_chunk->full_state |= world_full_chunk::state_en::generation_mark;
			to_mesh.emplace_back(meshing_data{ curr_nb->xz_offset, curr_nb->full_chunk, {} });
		}

		it = m_generator.reserved_map.erase(it);
	}

	// Wait for main thread to confirm the ability to mesh to avoid conflicts when editing
	if (!gen_thread_conditional_wait(gen_state_en::await_confirm)) return;

	// Mesh all affected chunks in parallel
	meshing_data *const affected_ptr = to_mesh.data();
	thread_ops::split(game.generation_thread_count, to_mesh.size(), [&](int, size_t index, size_t end) {
		quad_data_t *const full_quad_data = new quad_data_t[chunk_vals::total_faces];
		meshing_data *local_mesh_ptr = affected_ptr + index;
		const meshing_data *const local_end_ptr = affected_ptr + end;
	mesh_subchunks_loop:
		for (pos_t y_offset = 0; y_offset < chunk_vals::y_count; ++y_offset) {
			if (!game.is_active) goto immediate_end;
			local_mesh_ptr->full_chunk->subchunks[y_offset].mesh_faces(
				rendered_map,
				local_mesh_ptr->full_chunk,
				&local_mesh_ptr->full_offset,
				local_mesh_ptr->meshed_counters[y_offset],
				y_offset,
				full_quad_data
			);
		}
		if (++local_mesh_ptr != local_end_ptr) goto mesh_subchunks_loop;
	immediate_end:
		delete[] full_quad_data;
	});

	gen_thread_conditional_wait(gen_state_en::gen_finished); // Wait for a game exit or a generation event
} while (game.is_active);
}

bool world_obj::gen_thread_conditional_wait(gen_state_en new_state)
{
	static std::mutex generation_thread_mutex;
	std::unique_lock<std::mutex> lock(generation_thread_mutex);
	m_gen_thread_state = new_state;
	m_gen_conditional.wait(lock, [&]{ return m_gen_thread_state == gen_state_en::active || !game.is_active; });
	return game.is_active;
}

void world_obj::update_inst_buffer_data() noexcept
{
	game.perfs.buf_update.start_timer();
	existing_quads_count = 0;
	active_chunks.clear();

	// Determine total number of quads and which chunks are valid for rendering
	for (const auto &it : rendered_map) {
		const bool is_full_meshing = it.second->full_state & world_full_chunk::state_en::generation_mark;
		for (pos_t y_offset = 0; y_offset < chunk_vals::y_count; ++y_offset) {
			world_chunk *const chunk = it.second->subchunks + y_offset;
			if (!chunk->blocks) continue; // Ignore air chunks
			
			// Ignore chunks being meshed that do not have any previous data to use in the meantime
			if (is_full_meshing) {
				if (!(chunk->state & world_chunk::states_en::has_data_before)) continue;
				else chunk->state |= world_chunk::states_en::use_tmp_data;
			}

			// Accumulate counters from each chunk face
			const size_t prev_existing_count = existing_quads_count;
			for (int i = 0; i < 6; ++i) existing_quads_count += chunk->face_counters[i].total_faces();
			if (prev_existing_count == existing_quads_count) continue; // No faces present, ignore this chunk
			active_chunks.emplace_back(active_chunk_obj{ chunk, &it.first, y_offset }); // Add to active list
		}
	}

	// Bind world vertex array and instanced buffer and map instanced quad data buffer
	glBindVertexArray(m_world_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_world_inst_vbo);
	const uint32_t *const curr_inst_buffer = static_cast<uint32_t*>(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY));

	// Allocate the exact amount of data needed
	// TODO: realloc
	uint32_t *const new_data_ptr = new uint32_t[existing_quads_count];
	uint32_t new_index = 0;

	// Add all quad data to the above pointer from each valid chunk or its previously stored data
	for (auto &it : active_chunks) {
		const bool use_temp = it.chunk->state & world_chunk::states_en::use_tmp_data;
		for (int i = 0; i < 6; ++i) {
			const uint32_t dir_faces = it.chunk->face_counters[i].total_faces();
			const size_t dir_faces_bytes = sizeof(uint32_t) * dir_faces;
			const uint32_t *const curr_quad_ptr = it.chunk->quads_ptr[i];
			uint32_t *const quad_data_dst = new_data_ptr + new_index;
			uint32_t *const saved_data_index = it.chunk->glob_data_inds + i;

			if (!curr_quad_ptr || use_temp) { // No new data or being edited, use stored data from buffer
				::memcpy(quad_data_dst, curr_inst_buffer + *saved_data_index, dir_faces_bytes);
			} else { // New data available, use it instead and then clear
				::memcpy(quad_data_dst, curr_quad_ptr, dir_faces_bytes);
				delete[] curr_quad_ptr;
				it.chunk->quads_ptr[i] = nullptr;
			}

			it.chunk->state |= world_chunk::states_en::has_data_before; // Mark as 'has been updated before'
			*saved_data_index = new_index; // Set index for used data
			new_index += dir_faces;
		}
	}

	// Unmap instanced quad data buffer then buffer in the new data
	glUnmapBuffer(GL_ARRAY_BUFFER);
	if (!existing_quads_count) glBufferData(GL_ARRAY_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);
	else glBufferData(GL_ARRAY_BUFFER, sizeof(uint32_t) * existing_quads_count, new_data_ptr, GL_DYNAMIC_DRAW);

	delete[] new_data_ptr; // Delete created global quad array
	m_do_buffers_update = false;

	if (m_do_arrays_update) update_world_arrays(); // Update arrays if requested as well
	sort_world_render(); // Update rendered chunks and faces

	game.perfs.buf_update.end_timer();
}

void world_obj::update_world_arrays() noexcept
{
	// Total number of (sub)chunk faces (6 for each face of a cube)
	const size_t total_chunk_faces = get_chunks_count(true) * 6;

	// Delete any existing arrays (not UB deleting nullptr initially)
	// TODO: realloc
	delete[] m_translucent_faces;
	delete[] m_indirect_array;
	delete[] m_shader_offset_array;

	m_translucent_faces = new translucent_render_data[total_chunk_faces]; // Create array for chunk sorting with new size

	// Need double size as transparency needs to be rendered separately
	const size_t total_chunk_faces_w_trnsp = total_chunk_faces * 2;
	m_indirect_array = new indirect_cmd[total_chunk_faces_w_trnsp]; // Array for indirect command data (4 GLuints)
	m_shader_offset_array = new ssbo_offset_data[total_chunk_faces_w_trnsp]; // Array for offest SSBO data in shader

	// Ensure correct buffers are updated
	glBindVertexArray(m_world_vao);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_world_dib);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_world_ssbo);

	// Storage for draw commands and SSBO data
	glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(indirect_cmd) * total_chunk_faces_w_trnsp, nullptr, GL_DYNAMIC_DRAW);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo_offset_data) * total_chunk_faces_w_trnsp, nullptr, GL_DYNAMIC_COPY);

	m_do_arrays_update = false;
}

void world_obj::sort_world_render() noexcept
{
	game.perfs.render_sort.start_timer();

	// Offset value data
	ssbo_offset_data curr_offset_data;
	m_indirect_calls = 0;

	// Counter variables
	size_t translucent_count = 0;
	rendered_squares_count = 0;
	rendered_chunks_count = 0;

	// After storing the normal face data for every chunk, loop through the individual chunk faces
	// that have translucent faces, giving the offset and chunk data for each
	for (const auto &it : active_chunks) {
		const world_pos curr_offset = { it.xz_offset->x, it.y_offset, it.xz_offset->y };
		// Use frustum culling to determine if the chunk is on-screen
		const vector3d corner = curr_offset * chunk_vals::size; // Get chunk corner
		if (!world_plr->frustum.is_chunk_visible(corner)) continue;
		++rendered_chunks_count;
		
		// Set offset data for shader
		curr_offset_data.wrld_x = corner.x;
		curr_offset_data.wrld_z = corner.z;
		curr_offset_data.wrld_y = static_cast<float>(corner.y);
		
		for (curr_offset_data.face_ind = 0; curr_offset_data.face_ind < 6; ++curr_offset_data.face_ind) {
			const world_chunk::face_counts_obj *const face_counts = it.chunk->face_counters + curr_offset_data.face_ind;
			
			// Add transparent part of face at the end if any
			if (face_counts->translucent_count) m_translucent_faces[translucent_count++] = { curr_offset_data, it.chunk };
			if (!face_counts->opaque_count) continue; // Ignore if there are no normal faces

			// Check if it would even be possible to see this (opaque) face of the chunk
			// e.g. you can't see (relatively) forward faces in a chunk in front of you
			
			switch (curr_offset_data.face_ind) {
			case wdir_right:
				if (world_plr->offset.x < curr_offset.x) continue;
				break;
			case wdir_left:
				if (world_plr->offset.x > curr_offset.x) continue;
				break;
			case wdir_up:
				if (world_plr->offset.y < curr_offset.y) continue;
				break;
			case wdir_down:
				if (world_plr->offset.y > curr_offset.y) continue;
				break;
			case wdir_front:
				if (world_plr->offset.z < curr_offset.z) continue;
				break;
			case wdir_back:
				if (world_plr->offset.z > curr_offset.z) continue;
				break;
			default:
				break;
			}

			// Set indirect and offset data at the same indexes in both buffers
			m_indirect_array[m_indirect_calls] = {
				4, face_counts->opaque_count, 0,
				it.chunk->glob_data_inds[curr_offset_data.face_ind]
			};
			m_shader_offset_array[m_indirect_calls++] = curr_offset_data; // Advance to next indirect call
			rendered_squares_count += face_counts->opaque_count;
		}
	}

	// Loop through all of the sorted chunk faces with translucent faces and save the specific data
	for (size_t i = 0; i < translucent_count; ++i) {
		// Get chunk face data
		const translucent_render_data *const translucent_ptr = m_translucent_faces + i;
		const uint32_t face_ind = translucent_ptr->ssbo_data.face_ind;
		const world_chunk::face_counts_obj *const face_counters = translucent_ptr->chunk->face_counters + face_ind;

		// Same as above but with translucent data
		m_indirect_array[m_indirect_calls] = {
			4, face_counters->translucent_count,
			0, translucent_ptr->chunk->glob_data_inds[face_ind] + face_counters->opaque_count,
		};
		m_shader_offset_array[m_indirect_calls++] = translucent_ptr->ssbo_data;
		rendered_squares_count += face_counters->translucent_count;
	}

	// Bind buffers to edit
	glBindVertexArray(m_world_vao); 
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_world_ssbo);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_world_dib);
	
	// Update SSBO and indirect buffer with their respective data and sizes
	glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(indirect_cmd) * m_indirect_calls, m_indirect_array);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ssbo_offset_data) * m_indirect_calls, m_shader_offset_array);

	game.perfs.render_sort.end_timer();
}

int world_obj::fill_nearby_data(const world_pos *offset, nearby_data_obj *nearby_data, bool include_y) const noexcept
{
	int found = 0, dir_ind = -1;
	for (const world_pos &dir : chunk_vals::dirs_xyz) {
		++dir_ind; // Advance to next 'direction'
		if (!include_y && (dir.y != 0)) continue; // Ignore 'Y' offsets when choosing not to include them

		// Find chunk and check if it exists
		const world_pos curr_nearby_offset = *offset + dir;
		const world_xzpos curr_nearby_xz_offset = curr_nearby_offset.xz();
		world_full_chunk *full_chunk = find_full_chunk_at(&curr_nearby_xz_offset);
		if (!full_chunk) continue;

		// Add to nearby data
		nearby_data[found++] = {
			full_chunk->subchunks + curr_nearby_offset.y, full_chunk,
			static_cast<world_dir_en>(dir_ind), curr_nearby_offset
		};
	}

	return found; // Only loop through valid parts of the nearby data array
}

int world_obj::fill_nearby_full_data(const world_xzpos *offset, nearby_full_data_obj *nearby_full_data) const noexcept
{
	int found = 0, dir_ind = -1;
	for (const world_xzpos &dir : chunk_vals::dirs_xz) {
		++dir_ind; // Advance to next 'direction'

		// Find chunk and check if it exists
		const world_xzpos curr_nearby_offset = *offset + dir;
		world_full_chunk *full_chunk = find_full_chunk_at(&curr_nearby_offset);
		if (!full_chunk) continue;

		// Add to nearby data
		nearby_full_data[found++] = { full_chunk, static_cast<world_dir_en>(dir_ind), curr_nearby_offset };
	}

	return found; // Only loop through valid parts of the nearby data array
}

size_t world_obj::get_chunks_count(bool include_subchunks) const noexcept
{
	// Number of total chunks in render distance, calculated as (2 * n * (n + 1) + 1) * h
	// where n is the render distance and h is the number of (sub)chunks in a full chunk.
	// Only including subchunks if requested.
	const size_t crd = static_cast<size_t>(m_render_distance);
	const size_t pattern_count = (2 * crd * (crd + 1)) + 1;
	if (include_subchunks) return pattern_count * chunk_vals::y_count;
	else return pattern_count;
}

world_obj::~world_obj()
{
	// Stop generation thread
	m_gen_conditional.notify_all();
	m_generation_thread.join();

	// Delete all created chunks
	for (const auto &it : rendered_map) delete it.second;
	for (const auto &it : m_generator.reserved_map) delete it.second;
	
	// Delete created buffer objects
	const GLuint delete_buffers[] = { 
		m_world_ssbo, 
		m_world_dib, 
		m_world_inst_vbo, 
		m_world_plane_vbo,
		m_borders_ebo,
		m_borders_vbo,
	};
	glDeleteBuffers(static_cast<GLsizei>(math::size(delete_buffers)), delete_buffers);

	// Delete created VAOs
	const GLuint delete_vaos[] = { m_world_vao, m_borders_vao };
	glDeleteVertexArrays(static_cast<GLsizei>(math::size(delete_vaos)), delete_vaos);

	// Delete stored arrays
	delete[] m_translucent_faces;
	delete[] m_indirect_array;
	delete[] m_shader_offset_array;
}
