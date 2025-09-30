#pragma once
#ifndef _SOURCE_WORLD_CHUNK_HDR_
#define _SOURCE_WORLD_CHUNK_HDR_

#include "World/Generation/Perlin.hpp"
#include "World/Generation/Settings.hpp"

struct world_full_chunk;

struct world_chunk
{
public:
	typedef std::unordered_map<world_xzpos, world_full_chunk*, vec_hash> world_map;
	chunk_vals::blocks_array *blocks = nullptr;
	
	quad_data_t *quads_ptr[6];
	uint32_t glob_data_inds[6];
	struct face_counts_obj {
		uint32_t opaque_count, translucent_count;
		inline uint32_t total_faces() const noexcept { return opaque_count + translucent_count; }
	} face_counters[6];

	struct structure_info { structure_id id; vector3i8 start; vector3i32 extents; };
	std::vector<structure_info> structures;
	enum states_en : uint8_t {
		has_data_before = 1,
		use_tmp_data = 2
	};
	uint8_t state = 0;

	chunk_vals::blocks_array *allocate_blocks();
	void construct_blocks(const noise_object::block_noise *perlin_list, int local_y_offset);
	void mesh_faces(
		const world_map &chunks_map,
		const world_full_chunk *full_chunk,
		const world_xzpos *const xz_offset,
		face_counts_obj *const result_counters,
		pos_t y_offset,
		quad_data_t *const quads_results_ptr
	);
};

struct world_full_chunk {
	enum state_en : uint8_t { generation_mark = 1 };
	uint8_t full_state = 0;
	world_chunk subchunks[chunk_vals::y_count];
	~world_full_chunk();
};

#endif
