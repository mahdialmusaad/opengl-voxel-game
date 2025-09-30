#pragma once
#ifndef _SOURCE_WORLD_WLD_HEADER_
#define _SOURCE_WORLD_WLD_HEADER_

#include "Player/PlayerDef.hpp"
#include "Rendering/TextRenderer.hpp"

class world_obj
{
public:
	world_chunk::world_map rendered_map;
	noise_obj_list world_noise_objs;
	world_acc_player *world_plr;
	
	size_t existing_quads_count = 0, rendered_squares_count, rendered_chunks_count;

	world_obj(world_acc_player *player) noexcept;
	void draw_entire_world() noexcept;
	void draw_enabled_borders() noexcept;

	world_chunk *find_chunk_at(const world_pos *offset) const noexcept;
	world_full_chunk *find_full_chunk_at(const world_xzpos *offset) const noexcept;
	world_chunk *world_pos_to_chunk(const world_pos *position) const noexcept;

	block_id block_at(const world_pos *position) const noexcept;
	void set_block_at_to(const world_pos *position, block_id new_block_id) noexcept;

	pos_t highest_solid_y_at(world_xzpos *xz_position) const noexcept;

	void update_render_distance(int32_t new_rnd_dist) noexcept;
	bool xz_in_rnd_dist(const world_xzpos *chunk_offset) const noexcept;

	uintmax_t fill_blocks(world_pos from, world_pos to, block_id new_block_id) noexcept;

	void generation_loop(bool is_main_thread) noexcept;
	inline void signal_generation_thread() noexcept { m_do_gen_update = true; }

	void sort_world_render() noexcept;

	struct nearby_data_obj {
		world_chunk *chunk;
		world_full_chunk *full_chunk;
		world_dir_en dir_index;
		world_pos offset;
	};
	struct nearby_full_data_obj {
		world_full_chunk *full_chunk;
		world_dir_en dir_index;
		world_xzpos xz_offset;
	};

	int fill_nearby_data(const world_pos *offset, nearby_data_obj *nearby_data, bool include_y) const noexcept;
	int fill_nearby_full_data(const world_xzpos *offset, nearby_full_data_obj *nearby_full_data) const noexcept;

	GLsizei get_ind_calls() const noexcept { return static_cast<size_t>(m_indirect_calls); }
	int32_t get_rnd_dist() const noexcept { return m_render_distance; }
	size_t get_chunks_count(bool include_height = true) const noexcept;

	~world_obj();
private:
	quad_data_t *update_chunk_immediate(
		const world_full_chunk *const full_chunk,
		world_chunk *const updating_chunk,
		const world_xzpos *const xz_offset,
		pos_t y_offset,
		quad_data_t *mesh_data_array
	);

	struct active_chunk_obj { world_chunk *chunk; const world_xzpos *xz_offset; pos_t y_offset; };
	std::vector<active_chunk_obj> active_chunks;

	void update_inst_buffer_data() noexcept;
	void update_world_arrays() noexcept;

	GLuint m_borders_vao, m_borders_vbo, m_borders_ebo;
	GLuint m_world_vao, m_world_inst_vbo, m_world_plane_vbo;
	GLuint m_world_ssbo, m_world_dib;
	GLsizei m_indirect_calls;

	int32_t m_render_distance = 0;

	enum gen_state_en : uint8_t {
		both_finish,
		gen_finished,
		main_activate,
		active,
		await_confirm,
		active_immediate
	} m_gen_thread_state = gen_state_en::active;
	bool m_do_buffers_update = false, m_do_gen_update = false, m_do_arrays_update = false;

	bool gen_thread_conditional_wait(gen_state_en new_state);

	std::condition_variable m_gen_conditional;
	std::thread m_generation_thread;

	struct ssbo_offset_data {
		double wrld_x, wrld_z;
		float wrld_y;
		uint32_t face_ind;
	} *m_shader_offset_array = nullptr;
	struct indirect_cmd {
		GLuint count;
		GLuint inst_count;
		GLuint first;
		GLuint base_inst;
	} *m_indirect_array = nullptr;
	struct translucent_render_data {
		ssbo_offset_data ssbo_data;
		const world_chunk *chunk;
	} *m_translucent_faces = nullptr;

	struct world_chunk_generator {
	public:
		void generate_surrounding(const world_xzpos *const curr_xz_offset, int32_t curr_rnd_dist) noexcept;
		bool can_del_full_chunk(
			const world_full_chunk *const full_chunk,
			const world_xzpos *const full_chunk_xz_offset,
			const world_xzpos *const thread_plr_xz_offset,
			int32_t curr_rnd_dist
		);

		static constexpr int unseen_reserve_dist = 2;
		world_chunk::world_map reserved_map;
		
		world_obj *world;
	private:
		std::mutex reserved_access_mutex;

		struct full_chunk_result { world_full_chunk *const full_chunk; noise_object::block_noise *const noise_table; };
		full_chunk_result create_full_chunk(const world_xzpos *const xz_offset);
		world_chunk *create_or_get_chunk(const world_pos *const offset);
		world_chunk *get_chunk(const world_pos *const offset);
		
		block_id &create_block_ref(const world_pos *const position);

		inline block_id local_get(const world_pos *const position) {
			return create_block_ref(position);
		}
		inline void local_set(const world_pos *const position, block_id new_block_id) {
			create_block_ref(position) = new_block_id;
		}

		void natural_set(const world_pos *const position, const block_properties::block_attributes *block_properties);
		
		void fill_noise_table(noise_object::block_noise *const results, const world_xzpos *chunk_xz_pos) noexcept;
		int_fast64_t noise_hash(const noise_object::block_noise *noise) noexcept;
		bool noise_chance(int_fast64_t hash, int one_in) noexcept { return !(hash % one_in); }
		
		void gen_full_chunk(const world_xzpos *const xz_offset) noexcept;
		bool create_default_tree(
			world_chunk::structure_info *const structure_info,
			world_pos curr_world_pos,
			const noise_object::block_noise *const noise
		) noexcept;
	} m_generator;
};

#endif
