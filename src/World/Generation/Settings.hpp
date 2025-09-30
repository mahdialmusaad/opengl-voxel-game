#pragma once
#ifndef _SOURCE_GENERATION_SETTINGS_HDR_
#define _SOURCE_GENERATION_SETTINGS_HDR_

#include "Application/Definitions.hpp"

enum world_dir_en
{
	wdir_none  = -1, // none
	wdir_right =  0, // positive X
	wdir_left  =  1, // negative X
	wdir_up    =  2, // positive Y
	wdir_down  =  3, // negative Y
	wdir_front =  4, // positive Z
	wdir_back  =  5  // negative Z
};

// Change any when there are enough unique IDs
typedef uint8_t structure_id_t;
typedef uint8_t texture_id_t;
typedef uint8_t block_id_t;
typedef uint8_t items_id_t;

enum class block_id : block_id_t
{
	air,
	grass,
	dirt,
	stone,
	sand,
	water,
	log,
	leaves,
	planks,
	blocks_count,
};
enum class structure_id : structure_id_t
{
	premesh_face_counters,
	default_tree
};
enum class items_id : items_id_t {}; // Currently not in use

namespace block_properties {
	enum block_props_bool_en : bool
	{
		transp = true,
		opaque = false,
		solidY = true,
		solidN = false
	};
	enum block_props_light_stages_en : uint8_t
	{
		light0,
		light1,
		light2,
		light3,
		light4,
		light5,
		light6,
		light7,
	};

	enum texture_id : texture_id_t
	{
		grass_top,
		grass_side,
		dirt_face,
		stone_face,
		sand_face,
		water_face,
		log_inside,
		log_outside,
		leaves_face,
		planks_face,
		tex_none = 0,
	};

	struct mesh_attributes {		
		constexpr mesh_attributes(block_id b_id, bool hasTrnsp) noexcept : id(b_id), has_trnsp(hasTrnsp) {}
		mesh_attributes() noexcept = default;
		block_id id;
		bool has_trnsp;
	};

	typedef bool (*const render_check_t)(const mesh_attributes *const self, const mesh_attributes *const target);

	// Functions to determine face visibility next to another block
	#define VFUN_E(name)inline bool R_##name(const mesh_attributes *const,      const mesh_attributes *const       ) noexcept
	#define VFUN_Y(name)inline bool R_##name(const mesh_attributes *const,      const mesh_attributes *const target) noexcept
	#define VFUN_B(name)inline bool R_##name(const mesh_attributes *const self, const mesh_attributes *const target) noexcept

	VFUN_E(never) { return false; }
	VFUN_E(always) { return true; }

	VFUN_Y(normal) { return target->has_trnsp; }
	VFUN_B(hide_self) { return self->id != target->id && target->has_trnsp; }

	#undef VFUN_E
	#undef VFUN_Y
	#undef VFUN_B

	struct block_attributes
	{
		constexpr block_attributes(
			block_id b_id,
			const char *block_name,
			texture_id t0, texture_id t1, texture_id t2, texture_id t3, texture_id t4, texture_id t5,
			block_props_bool_en has_transparency,
			block_props_bool_en solidness,
			block_props_light_stages_en light_stage,
			uint8_t block_strength,
			render_check_t render_function
		) noexcept :
			visible_with(render_function),
			name(block_name),
			mesh_info{ b_id, has_transparency },
			textures{ t0, t1, t2, t3, t4, t5 },
			emission(light_stage),
			strength(block_strength),
			solid(solidness)
		{}

		render_check_t visible_with;
		const char *name;
		mesh_attributes mesh_info;
		texture_id_t textures[6];
		uint8_t emission, strength;
		bool solid;
		bool padded[5]{};
	// Property definitions for all object IDs as seen above
	} constexpr const global_properties[static_cast<int>(block_id::blocks_count)] {
		{
			block_id::air, "Air",
			tex_none, tex_none, tex_none, tex_none, tex_none, tex_none, 
			transp, solidN, light0, 0, R_always
		},
		{ 
			block_id::grass, "Grass",
			grass_side, grass_side, grass_top, dirt_face, grass_side, grass_side, 
			opaque, solidY, light0, 8, R_normal
		},
		{
			block_id::dirt, "Dirt",
			dirt_face, dirt_face, dirt_face, dirt_face, dirt_face, dirt_face,
			opaque, solidY, light0, 7, R_normal
		},
		{
			block_id::stone, "Stone",
			stone_face, stone_face, stone_face, stone_face, stone_face, stone_face,
			opaque, solidY, light0, 30, R_normal
		},
		{
			block_id::sand, "Sand",
			sand_face, sand_face, sand_face, sand_face, sand_face, sand_face,
			opaque, solidY, light0, 6, R_normal
		},
		{
			block_id::water, "Water",
			water_face, water_face, water_face, water_face, water_face, water_face,
			transp, solidN, light0, 0, R_hide_self
		},
		{
			block_id::log, "Log",
			log_outside, log_outside, log_inside, log_inside, log_outside, log_outside,
			opaque, solidY, light0, 12, R_normal
		},
		{
			block_id::leaves, "Leaves",
			leaves_face, leaves_face, leaves_face, leaves_face, leaves_face, leaves_face,
			transp, solidY, light0, 3, R_hide_self
		},
		{
			block_id::planks, "Planks",
			planks_face, planks_face, planks_face, planks_face, planks_face, planks_face,
			opaque, solidY, light0, 10, R_normal
		}
	};

	constexpr const block_attributes *of_block(block_id b_id) noexcept {
		return global_properties + static_cast<int>(b_id);
	}
	constexpr const mesh_attributes *mesh_of_block(block_id b_id) noexcept {
		return &(global_properties + static_cast<int>(b_id))->mesh_info;
	}
};

// Game settings
namespace chunk_vals
{
	// Chunk generation settings (editable)
	constexpr int size = 32; // The side length of a chunk cube.
	constexpr int y_count = 8; // The number of subchunks in a full chunk.
	
	constexpr int base_dirt = 3; // Amount of dirt blocks between surface and stone.
	constexpr int water_y = 80; // Maximum Y position of water.
	constexpr int tree_spawn_chance = 100; // The chance for a grass block to have a tree.
	
	constexpr double noise_step = static_cast<double>(size) / 128.0; // How much to traverse in the 'noise map' per chunk.
	constexpr float max_terrain = 200.0f; // Maximum height for surface.
	constexpr float min_surface = 50.0f; // Minimum height for surface (including underwater surface).
	
	// Calculation shortcuts - do not change.
	constexpr int world_height = size * 8;
	constexpr int top_y_ind = y_count - 1;
	constexpr int less = size - 1;
	constexpr double sqrt_size = math::cxsqrt(static_cast<double>(size));
	constexpr float surface_range = max_terrain - min_surface;
	constexpr float half = size / 2.0f;
	constexpr int size_bits = math::bits(less);
	constexpr int full_bits = ((~0u) >> ((sizeof(GLuint) * CHAR_BIT) - size_bits));
	
	constexpr int32_t squared = static_cast<int32_t>(size) * size;
	constexpr int32_t blocks_count = squared * size;
	constexpr int32_t total_faces = blocks_count * 6;

	constexpr world_pos dirs_xyz[6] = {
		{  1,  0,  0  },   // X+
		{ -1,  0,  0  },   // X-
		{  0,  1,  0  },   // Y+
		{  0, -1,  0  },   // Y-
		{  0,  0,  1  },   // Z+
		{  0,  0, -1  }    // Z-
	};
	constexpr world_xzpos dirs_xz[4] = {
		{  1,  0  },   // X+
		{ -1,  0  },   // X-
		{  0,  1  },   // Z+
		{  0, -1  }    // Z-
	};
	
	extern uint32_t faces_lookup[total_faces];  // First 3 bits of each represent face index, rest determine block index

	typedef block_id (blocks_array[chunk_vals::size][chunk_vals::size][chunk_vals::size]);

	void fill_lookup() noexcept;

	pos_t to_block_pos_one(double x) noexcept;
	pos_t to_block_pos_one(float x) noexcept;
	template<typename T> pos_t to_block_pos_one(T a) noexcept { return static_cast<pos_t>(a); }
	template<typename T> world_pos to_block_pos(const vector<T, 3> *vec) noexcept {
		return vector<T, 3>(to_block_pos_one(vec->x), to_block_pos_one(vec->y), to_block_pos_one(vec->z) );
	}
	
	template<typename T> pos_t world_to_offset_one(T x) noexcept {
		const pos_t v = to_block_pos_one(x);
		const pos_t a = v < 0 ? v - static_cast<pos_t>(less) : v;
		return (a - (a % static_cast<pos_t>(size))) / static_cast<pos_t>(size);
	}
	template<typename T> world_pos world_to_offset(const vector<T, 3> *pos) noexcept {
		return vector<T, 3>(world_to_offset_one(pos->x), world_to_offset_one(pos->y), world_to_offset_one(pos->z));
	}
	template<typename T> world_xzpos world_to_offset(const vector<T, 2> *pos) noexcept {
		return vector<T, 2>(world_to_offset_one(pos->x), world_to_offset_one(pos->y));
	}
	
	template<typename T> int world_to_local_one(T x) noexcept {
		return static_cast<int>(static_cast<intmax_t>(x) - (static_cast<intmax_t>(size) * world_to_offset_one(x)));
	}
	template<typename T> vector3i world_to_local(const vector<T, 3> *pos) noexcept {
		return { world_to_local_one(pos->x), world_to_local_one(pos->y), world_to_local_one(pos->z) };
	}
	template<typename T> vector2i world_to_local(const vector<T, 2> *pos) noexcept {
		return { world_to_local_one(pos->x), world_to_local_one(pos->y) };
	}
	
	bool is_bordering(const vector3i &pos, world_dir_en dir) noexcept;
	bool is_y_outside_bounds(pos_t y) noexcept;
	pos_t offsets_dist(const world_xzpos *off_a, const world_xzpos *off_b) noexcept;

	int step_dir_to(pos_t start, pos_t end) noexcept;
	vector3i step_dir_to(const world_pos *start, const world_pos *end) noexcept;
	vector2i step_dir_to(const world_xzpos *start, const world_xzpos *end) noexcept;

	static_assert(size > 0, "Side length must be valid.");
	static_assert(y_count <= 256, "Too many subchunks (>256).");
	static_assert(world_height >= size, "The world height must be >= chunk size.");
	static_assert(!(world_height % size), "The world height must be a multiple of the chunk size.");
};

// Ranges for certain values across the game
namespace val_limits
{
	template<typename T> struct lim_vec { T min, max; };
	constexpr lim_vec<double> fov_limits  = { 20.0, 120.0 };
	constexpr lim_vec<int32_t> render_limits  = { 4, 200 };
	constexpr lim_vec<double> tick_limits = { -200.0, 200.0 };
}

#endif // _SOURCE_GENERATION_SETTINGS_HDR_
