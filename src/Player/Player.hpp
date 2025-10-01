#pragma once
#ifndef SOURCE_PLAYER_PLR_VXL_HDR
#define SOURCE_PLAYER_PLR_VXL_HDR

#include "World/World.hpp"

// Full player class including 'world-visible' player class
class player_inst
{
private:
	GLuint m_outline_vao, m_outline_vbo, m_outline_ebo;
	GLuint m_inventory_vao, m_inventory_inst_vbo, m_inventory_quad_vbo;

	uint8_t m_hotbar_instances, m_inv_instances;
	uint8_t m_selected = 0;

	vector3i8 place_rel_pos;

	text_renderer_obj::text_obj m_block_text;
	text_renderer_obj::text_obj m_hotbar_text[9];
	text_renderer_obj::text_obj m_slot_text[sizeof(world_acc_player::inventory) / sizeof(world_acc_player::inv_slot_obj)];
public:
	world_acc_player player;
	world_obj *world;
	
	enum class player_movement_en { positive_z, negative_z, negative_x, positive_x, flying_up, flying_down, jumping };

	player_inst() noexcept;
	void init_text() noexcept;
	void check_movement_input() noexcept;

	void apply_velocity() noexcept;
	void set_new_position(const vector3d &new_pos) noexcept;
	inline const vector3d &get_current_velocity() const noexcept { return m_velocity; }

	void break_selected() noexcept;
	void place_selected() noexcept;

	void update_cam_dir_vars(double x = 0.0, double y = 0.0) noexcept;
	void add_velocity(player_movement_en move) noexcept;

	int find_slot_with_item(block_id item, bool include_full_slots) noexcept;
	int find_free_or_matching_slot(block_id item) noexcept;

	void draw_selected_outline() const noexcept;
	void draw_inventory_gui() noexcept;
	
	inline matrixf4x4 get_zero_matrix() const noexcept {
		return matrixf4x4::look_at({ 0.0, 0.0, 0.0 }, vector3f(m_cam_front), vector3f(m_cam_up));
	}

	void update_plr_offset() noexcept;
	inline void update_frustum_vals() noexcept {
		player.frustum.update_frustum_vals(player.position, m_cam_front, m_cam_up, m_cam_right, player.fov);
	}

	void upd_inv_scroll_relative(float y_offset) noexcept;
	void upd_inv_selected(int slot_ind) noexcept;

	void update_inventory_buffers() noexcept;
	void update_item_text_pos() noexcept;
	void update_slot(int slot_ind, block_id b_id, uint8_t count);
	
	void update_selected_block_info_txt() noexcept;

	~player_inst();
private:
	void pos_changed_vars_upd() noexcept;
	void upd_cam_vectors() noexcept;
	void upd_raycast_selected() noexcept;

	// 5 rows * 9 columns * 2 (either normal slot or block texture) + 2 (crosshair and background)
	static constexpr int inventory_elems_count = (9 * 5 * 2) + 2;
	struct inventory_inst {
		inventory_inst(
			vector4f dimensions = {}, uint32_t texture = 0u, bool is_block_tex = false
		) noexcept : dims(dimensions), texid(static_cast<int>(is_block_tex) + (texture << 1)) {}

		vector4f dims;
		uint32_t texid;
	} *inventory_data = new inventory_inst[inventory_elems_count];

	enum class slot_type_en { hotbar, inventory, slot_size, slot_block_size };

	vector2f get_slot_pos(slot_type_en stype, int id, bool is_block) const noexcept;
	vector4f get_slot_dims(slot_type_en stype, int id, bool is_block) const noexcept;

	vector3d m_cam_up, m_cam_front, m_cam_right;
	vector3d m_NY_cam_front, m_NY_cam_right;
	vector3d m_velocity;
};

#endif // SOURCE_PLAYER_PLR_VXL_HDR
