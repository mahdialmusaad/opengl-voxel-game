#include "Chunk.hpp"

void world_chunk::construct_blocks(const noise_object::block_noise *perlin_ptr, int y_offset)
{
	// Determine the starting Y position of this chunk
	const int world_y_pos = y_offset * chunk_vals::size;

	// The chunk has not been created yet so initially use a full array
	// and use an air chunk instead if no blocks are present after creation
	allocate_blocks();

	int32_t air_count = 0;
	const noise_object::block_noise *current_noise = perlin_ptr;

	block_id *const blocks_ptr = blocks[0][0][0];

	for (int ind = 0; ind < chunk_vals::squared; ++ind) {
		const int xz_ind = (ind % chunk_vals::size) + ((ind / chunk_vals::size) * chunk_vals::squared);
		// Chunks with the same XZ offset will use the same positions for the perlin noise calculation
		// and therefore will get the same result each time, so it's much better to reuse it
		const int terrain_height = static_cast<int>(current_noise++->height);

		// Loop through Y axis
		for (int y = 0; y < chunk_vals::size; ++y) {
			const int curr_world_y = world_y_pos + y;
			block_id to_place = block_id::grass;

			if (curr_world_y == terrain_height) { // Block is at the surface
				// Blocks under or at the water level are sand otherwise the default ground block
				to_place = curr_world_y <= chunk_vals::water_y ? block_id::sand : to_place;

			} else if (curr_world_y < terrain_height) { // Block is under surface
				// Blocks slightly under surface are dirt but otherwise stone
				to_place = terrain_height - curr_world_y <= chunk_vals::base_dirt ? block_id::dirt : block_id::stone;
			} else { // Block is above surface
				// Fill low surfaces with water
				if (curr_world_y < chunk_vals::water_y) to_place = block_id::water;
				// Rest of the blocks are air as this is above the 'height', add to air counter
				else { air_count += chunk_vals::size - y; break; }
			}

			blocks_ptr[xz_ind + (y * chunk_vals::size)] = to_place; // Set block using 1D index
		}
	}

	// If the chunk is just air, there is no need to store every single block
	if (air_count == chunk_vals::blocks_count) {
		::free(blocks);
		blocks = nullptr;
	}
}

void world_chunk::mesh_faces(
	const world_map &chunks_map,
	const world_full_chunk *,
	const world_xzpos *const xz_offset,
	face_counts_obj *const result_counters,
	pos_t y_offset,
	quad_data_t *const quads_results_ptr
) {
	if (!blocks) return; // Don't calculate air chunks
	::memset(&quads_ptr, 0, sizeof quads_ptr); // Reset all quad data
	
	const block_id *const block_start_ptr = blocks[0][0][0]; // Use 1D array access instead of 3D for speed

	// Store nearby chunks in an array for easier access (last index is current chunk)
	const block_id *nearby_ptrs[7] {
		nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr,
		block_start_ptr
	};

	// Determine above and below chunks from memory/'this' address
	// since chunks are stored contiguously in the 'subchunk array'
	if (y_offset && this[-1].blocks) nearby_ptrs[wdir_down] = this[-1].blocks[0][0][0];
	if (y_offset != chunk_vals::top_y_ind && this[1].blocks) nearby_ptrs[wdir_up] = this[1].blocks[0][0][0];

	// Get nearby chunks
	for (int i = 0; i < 4; ++i) {
		const world_xzpos new_offset = *xz_offset + chunk_vals::dirs_xz[i]; // Calculate nearby XZ offset
 		const auto it = chunks_map.find(new_offset); // Look for a full chunk struct with the calculate offset
		// Add possible adjacent chunk if one is found with valid blocks
		if (it != chunks_map.end() && it->second->subchunks[y_offset].blocks)
		nearby_ptrs[i + ((i >= wdir_up) * 2)] =
		        it != chunks_map.end() && it->second->subchunks[y_offset].blocks ?
		              it->second->subchunks[y_offset].blocks[0][0][0] :
		              nullptr;
	}

	// Saved lookup data and chunk indexes
	const uint32_t *curr_lookup_ptr = chunk_vals::faces_lookup;
	for (uint32_t block_index = 0; block_index < chunk_vals::blocks_count; ++block_index) {
		// Get block ID from current pointer index into blocks array
		const block_id curr_block_id = block_start_ptr[block_index];
		if (curr_block_id == block_id::air) { // Skip if air is found - never rendered
			curr_lookup_ptr += 6;
			continue;
		}

		// Get data on current block
		const block_properties::block_attributes *const curr_attributes = block_properties::of_block(curr_block_id);
		// Values used to determine visibility
		const block_properties::mesh_attributes *const curr_mesh_data = &curr_attributes->mesh_info;
		// Get specific visibility check function
		const block_properties::render_check_t visible_with = curr_attributes->visible_with;

		// Check for visibility against comparing block and add face if it can be seen for each face of the block
		for (int i = 0; i < 6; ++i) {
			const uint32_t lookup_int = *curr_lookup_ptr++;
			const block_id *const block_ptr = nearby_ptrs[lookup_int & 7];
			if (!visible_with(
				curr_mesh_data,
				block_properties::mesh_of_block(block_ptr ? block_ptr[lookup_int >> 3] : block_id::air)
			)) continue;
			// Compress the position and texture data into one integer
			// Layout: TTTT TTTT TTTT TTTT TZZZ ZZYY YYYX XXXX
			const uint32_t z_pos = block_index % chunk_vals::size;
			const uint32_t x_pos = (block_index / chunk_vals::squared);
			const uint32_t y_pos = (block_index / chunk_vals::size) % chunk_vals::size;

			const quad_data_t quad_data = 
			    x_pos + (y_pos << chunk_vals::size_bits) + (z_pos << (chunk_vals::size_bits * 2)) + // Position in chunk
			    (static_cast<uint32_t>(curr_attributes->textures[i]) << (chunk_vals::size_bits * 3)); // Texture

			// Blocks with transparency need to be rendered last for them to be rendered
			// correctly on top of existing terrain, so they can be placed starting from
			// the end of the data array instead to be separated from the opaque blocks.
			
			// For translucent faces, set the data in reverse order, starting from the end of
			// the array (pre-increment to avoid writing to out of bounds the first time).
			// If it is a normal face however, just add to the array normally.
			face_counts_obj *const face_dir_counter = result_counters + i;
			quads_results_ptr[(curr_mesh_data->has_trnsp ?
				chunk_vals::blocks_count - ++face_dir_counter->translucent_count :
				face_dir_counter->opaque_count++) + (i * chunk_vals::blocks_count)
			] = quad_data;
		}
	}

	// Remove possible gap between the two face counters for each 'chunk face'
	for (int face_ind = 0; face_ind < 6; ++face_ind) {
		const face_counts_obj *const face_dir_counter = result_counters + face_ind;
		const uint32_t total_faces_count = face_dir_counter->total_faces();
		if (!total_faces_count) continue;

		// Allocate the exact amount of data needed to store all the faces
		quad_data_t *const compressed_quads = new quad_data_t[total_faces_count];
		
		// Below data probably looks like this:
		// [(opaque data)(  gap  )(translucent data)]
		const uint32_t *const face_quads_ptr = quads_results_ptr + (face_ind * chunk_vals::blocks_count);
		
		// Copy the opaque and translucent sections of the face data to be 
		// next to each other in the new compressed array

		// Opaque face data
		::memcpy(compressed_quads, face_quads_ptr, sizeof(quad_data_t) * face_dir_counter->opaque_count);
		::memcpy(compressed_quads + face_dir_counter->opaque_count,
		       face_quads_ptr + (chunk_vals::blocks_count - face_dir_counter->translucent_count),
		       sizeof(quad_data_t) * face_dir_counter->translucent_count
		);

		// The data now is stored as such (assuming no bugs):
		// [(opaque data)(translucent data)] - No gaps: exact size is allocated
		quads_ptr[face_ind] = compressed_quads;
	}
}

chunk_vals::blocks_array *world_chunk::allocate_blocks()
{
	// Allocate memory for chunk blocks array if it does not already exist
	if (blocks) return blocks;
	return blocks = static_cast<chunk_vals::blocks_array*>(::calloc(1, sizeof *blocks));
}

// Delete block array in all chunks
world_full_chunk::~world_full_chunk() { for (world_chunk &chunk : subchunks) if (chunk.blocks) ::free(chunk.blocks); }
