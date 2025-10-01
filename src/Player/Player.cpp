#include "Player.hpp"

player_inst::player_inst() noexcept
{
	// Setup VAO and VBO for selection outline
	m_outline_vao = ogl::new_vao();
	glEnableVertexAttribArray(0);
	
	// To ensure that the block selection outline is always visible, a slightly *smaller* square outline 
	// is rendered slightly *in front* of each block face to avoid Z-fighting and block clipping
	const float deviance = 0.001f;
	const float front1 = 1.0f + deviance, front0 = deviance, small1 = 1.0f - deviance, small0 = -deviance;
	const float outline_cube_coords[] = {
		// Top face
		small0, front1, small0,
		small1, front1, small0,
		small1, front1, small1,
		small0, front1, small1,
		// Bottom face
		small1, front0, small0,
		small0, front0, small0,
		small0, front0, small1,
		small1, front0, small1,
		// Right face
		front1, small0, small1,
		front1, small0, small0,
		front1, small1, small0,
		front1, small1, small1,
		// Left face
		front0, small0, small1,
		front0, small0, small0,
		front0, small1, small0,
		front0, small1, small1,
		// Front face
		small0, small1, front1,
		small0, small0, front1,
		small1, small0, front1,
		small1, small1, front1,
		// Back face
		small0, small1, front0,
		small0, small0, front0,
		small1, small0, front0,
		small1, small1, front0,
	};

	// VBO with line float data
	m_outline_vbo = ogl::new_buf(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof outline_cube_coords, outline_cube_coords, 0);

	// Line indexes to determine positions of each line that makes up the outline cube
	const uint8_t selected_outline_inds[] = {
		0,  1,  1,  2,  2,  3,  3,  0,	// Right face
		4,  5,  5,  6,  6,  7,  7,  4,	// Left face
		8,  9,  9,  10, 10, 11, 11, 8,	// Top face	
		12, 13, 13, 14, 14, 15, 15, 12,	// Bottom face
		16, 17, 17, 18, 18, 19, 19, 16,	// Front face
		20, 21, 21, 22, 22, 23, 23, 20	// Back face
	};

	m_outline_ebo = ogl::new_buf(GL_ELEMENT_ARRAY_BUFFER);
	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof selected_outline_inds, selected_outline_inds, 0);

	m_inventory_vao = ogl::new_vao(); // Setup VAO for inventory

	// Enable attributes 0-2: 0 = vec4, 1 = int, 2 = base vec3
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	// Attribute divisors for instanced rendering
	glVertexAttribDivisor(0, 1);
	glVertexAttribDivisor(1, 1);

	m_inventory_inst_vbo = ogl::new_buf(GL_ARRAY_BUFFER); // VBO for the inventory texture data

	// Attribute layout (location 0 and 1)
	// A0 (vec4): X, Y, W, H
	// A1 (int): Texture type - text or block (1st bit), inner texture id (other bits)
	constexpr GLsizei stride = sizeof(float[4]) + sizeof(int32_t);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, nullptr);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, stride, reinterpret_cast<const void*>(sizeof(float[4])));

	// Inventory quad vertex data
	const vector3f quad_vert_data[4] = {
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f },
		{ 1.0f, 1.0f, 0.0f },
		{ 1.0f, 0.0f, 1.0f }
	};

	// VBO for the quad vertices for inventory elements (location 2)
	m_inventory_quad_vbo = ogl::new_buf(GL_ARRAY_BUFFER);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof quad_vert_data, quad_vert_data, 0);

	upd_cam_vectors();
	update_plr_offset();
}

void player_inst::init_text() noexcept
{
	// Initialize variables that require other classes

	// Selected block information text - created in player class instead of
	// with other text objects so it can be updated only when the selected block changes
	const float default_size = text_renderer_obj::text_obj::default_font_size;
	::glob_txt_rndr->init_text_obj(&m_block_text,
		{}, "", text_renderer_obj::ts_shadow | text_renderer_obj::ts_bg | text_renderer_obj::ts_is_debug
	);
	
	update_cam_dir_vars(); // Update yaw/pitch and raycast block which updates above text

	// Inventory item counters - hotbar slots have a mirrored one in the inventory
	for (int slot_ind = 0; slot_ind < 36; ++slot_ind) {
		world_acc_player::inv_slot_obj *const slot = player.inventory + slot_ind;
		text_renderer_obj::text_obj *const slot_text = m_slot_text + slot_ind;
		const std::string text = slot->count ? std::to_string(slot->count) : "";

		// Position is determined in update_item_text_pos function, leave empty for now
		::glob_txt_rndr->init_text_obj(slot_text, {}, text,
			text_renderer_obj::ts_shadow | (text_renderer_obj::ts_in_inv),
			default_size - 2.0f, slot->count
		);

		// Also create copy of inventory hotbar for normal hotbar
		if (slot_ind < 9) {
			text_renderer_obj::text_obj *const linked_hotbar_text = m_hotbar_text + slot_ind;
			::glob_txt_rndr->init_text_obj(linked_hotbar_text, slot_text);
		}
	}

	update_item_text_pos(); // Update text positions
}

void player_inst::check_movement_input() noexcept 
{
	// Check for any input that requires holding
	
	// Get key state
	const auto key_active = [](int key) {
		return glfwGetKey(game.window_ptr, key) == GLFW_PRESS;
	};

	// Only check if there is a key being pressed and if the player isn't chatting or in inventory
	if (game.any_key_active && !game.chatting && !player.inventory_open) {
		// Movement inputs
		if (key_active(GLFW_KEY_W)) add_velocity(player_movement_en::positive_z);
		if (key_active(GLFW_KEY_S)) add_velocity(player_movement_en::negative_z);
		if (key_active(GLFW_KEY_A)) add_velocity(player_movement_en::negative_x);
		if (key_active(GLFW_KEY_D)) add_velocity(player_movement_en::positive_x);

		bool ctrl_input_read = false;
		if (player.gravity_effects) {
			// Jumping movement
			if (key_active(GLFW_KEY_SPACE)) add_velocity(player_movement_en::jumping);
		} else {
			// Flying mode inputs
			if (key_active(GLFW_KEY_SPACE)) add_velocity(player_movement_en::flying_up);
			if (key_active(GLFW_KEY_LEFT_CONTROL)) {
				add_velocity(player_movement_en::flying_down);
				ctrl_input_read = true;
			}
		}

		// Speed modifiers
		if (key_active(GLFW_KEY_LEFT_SHIFT)) player.active_speed = player.base_speed * 1.5;
		else if (!ctrl_input_read && key_active(GLFW_KEY_LEFT_CONTROL)) player.active_speed = player.base_speed * 0.5;
		else player.active_speed = player.base_speed;
	}
}

void player_inst::apply_velocity() noexcept
{
	game.perfs.movement.start_timer();

	// Make player slowly come to a stop instead of immediately stopping when no movement is applied
	// TODO: Framerate-independent lerp/smoothing
	const double move_intensity = math::clamp(game.frame_delta_time * 10.0, 0.0, 1.0);
	
	// Apply gravity if enabled
	const double gravity = -1.0, terminal_velocity = -10.0;
	if (player.gravity_effects) {
		m_velocity.y = math::max(m_velocity.y + (gravity * game.frame_delta_time), terminal_velocity);
	} else m_velocity.y = math::lerp(m_velocity.y, 0.0, move_intensity);

	// Smoothly stop movement
	m_velocity.x = math::lerp(m_velocity.x, 0.0, move_intensity);
	m_velocity.z = math::lerp(m_velocity.z, 0.0, move_intensity);
	
	if (m_velocity.sqr_length() < 0.001) return; // Treat near-zero velocity the same as no movement

	// Positions and axis direction list
	constexpr double ext = 0.2;
	struct col_check_obj {
		constexpr col_check_obj(double _x, double _z) noexcept :
		   x(_x), z(_z), apply_x(_x != 0.0), apply_z(_z != 0.0) {}
		double x, z; bool apply_x, apply_z;
	} static const perm_col_checks[] = {
		{  0.0,  0.0  }, // Y only
		{  ext,  0.0  }, // Positive X
		{ -ext,  0.0  }, // Negative X
		{  0.0,  ext  }, // Positive Z
		{  0.0, -ext  }, // Negative Z
		{ -ext,  ext  }, // Forward left
		{  ext,  ext  }, // Forward right
		{  ext, -ext  }, // Backward right
		{ -ext, -ext  }, // Backward left
	};
	
	// Reset specific velocity values if they make the player intersect with a block;
	// reset Y velocity if a block is detected below or above instead

	enum velocity_reset_en { VR_none, VR_x, VR_z, VR_xz } reset_state = VR_none;
	world_pos collided_pos;

	const auto reset_vels = [&](const col_check_obj &col) {
		// Only change velocity if its sign matches with the direction checked:
		// e.g. if the player moves in the +X direction, a block close 
		// enough in the -X direction should not stop their movement.
		
		// TODO: Proper velocity value after collision

		// X axis check
		if (col.apply_x && signbit(m_velocity.x) == signbit(col.x)) {
			m_velocity.x = 0.0;
			if (reset_state == VR_z) {
				reset_state = VR_xz;
				return true;
			} else {
				reset_state = VR_x;
				return false;
			}
		}

		// Z axis check
		if (col.apply_z && signbit(m_velocity.z) == signbit(col.z)) {
			m_velocity.z = 0.0;
			if (reset_state == VR_x) {
				reset_state = VR_xz;
				return true;
			} else {
				reset_state = VR_z;
				return false;
			}
		}

		return false;
	};
	
	const auto check_dirs = [&](const vector3d &new_pos, double ypos, bool reset_y, bool do_grounded) {
		bool first = true;
		for (const col_check_obj &col : perm_col_checks) {
			// The 'Y only' check only applies to ground and ceiling checks
			if (!reset_y && first) { first = false; continue; }
			const vector3d col_pos = new_pos + vector3d(col.x, ypos, col.z);
			collided_pos = chunk_vals::to_block_pos(&col_pos); // World position of block to check
			if (!block_properties::of_block(world->block_at(&collided_pos))->solid) continue; // Check for a solid block

			// Reset Y velocity and possibly change grounded state for floor and ceiling checks
			if (reset_y) {
				m_velocity.y = 0.0;
				if (do_grounded) player.grounded = true;
				break;
			}

			// Stop early if X and Z velocities are affected
			if (reset_vels(col)) break;
		}
	};

	constexpr double above_block_rel_pos = 0.2;
	constexpr double ground_rel_pos = -2.0 + above_block_rel_pos;
	constexpr double legs_rel_pos = ground_rel_pos + above_block_rel_pos;

	player.grounded = false;

	// Ignore collision checks if noclip is enabled
	if (!player.ignore_collision) {
		const vector3d new_pos = player.position + m_velocity; // The new position to check collisions for
		// All collision checks
		check_dirs(new_pos, above_block_rel_pos, true, false); // Blocks above player
		check_dirs(new_pos, ground_rel_pos, true, true); // Blocks on the ground beneath
		check_dirs(new_pos, legs_rel_pos, false, false); // Blocks around 'legs' position
		// Blocks around camera position (only if velocity hasn't been reset already)
		if (reset_state != VR_xz) check_dirs(new_pos, 0.0, false, false);
	}

	player.position += m_velocity; // Add new velocity to position
	pos_changed_vars_upd(); // Update variables and other data affected by position

	game.perfs.movement.end_timer();
}

void player_inst::set_new_position(const vector3d &new_position) noexcept
{
	// Set the new position of the player
	player.position = new_position;
	pos_changed_vars_upd();
}

void player_inst::pos_changed_vars_upd() noexcept
{
	player.moved = true;

	// Determine block at head position (air, water, etc) and the one the player is standing on
	const world_pos plr_block_pos = chunk_vals::to_block_pos(&player.position);
	player.head_block = world->block_at(&plr_block_pos);
	const world_pos feet_block_pos = { plr_block_pos.x, plr_block_pos.y - 2, plr_block_pos.z} ;
	player.feet_block = world->block_at(&feet_block_pos);

	update_frustum_vals();
	update_plr_offset();
	upd_raycast_selected();
}

void player_inst::break_selected() noexcept
{
	// Check if the selected block is solid/valid
	if (block_properties::of_block(player.target_block_id)->solid) {
		// Set broken block to air and redo raycast
		world->set_block_at_to(&player.selected_block_pos, block_id::air);
		const int slot_ind = find_free_or_matching_slot(player.target_block_id);
		if (slot_ind != -1) update_slot(slot_ind, player.target_block_id, player.inventory[slot_ind].count + 1);
		upd_raycast_selected();
	}
}

void player_inst::place_selected() noexcept
{
	// Determine selected block in inventory
	world_acc_player::inv_slot_obj &use_slot = player.inventory[m_selected];
	const block_id to_place_id = use_slot.held_id;

	// Only place if the current selected block is valid
	if (to_place_id != block_id::air && block_properties::of_block(player.target_block_id)->solid) {
		const world_pos place_pos = player.selected_block_pos + world_pos(place_rel_pos);
		if (block_properties::of_block(world->block_at(&place_pos))->solid) return; // Don't replace already existing blocks

		const world_pos plr_block_pos = chunk_vals::to_block_pos(&player.position);
		const world_pos plr_legs_block_pos = { plr_block_pos.x, plr_block_pos.y - static_cast<pos_t>(1), plr_block_pos.z };
		if (place_pos == plr_block_pos || place_pos == plr_legs_block_pos) return; // Don't place blocks inside the player

		world->set_block_at_to(&place_pos, to_place_id); // Place block on side of selected block
		update_slot(m_selected, to_place_id, use_slot.count - 1); // Update inventory slot with new count
		upd_raycast_selected(); // Raycast again to update selection
	}
}

void player_inst::add_velocity(player_movement_en direction) noexcept
{
	const double direction_mult = player.active_speed * game.frame_delta_time;

	// Add to velocity based on frame time and input
	switch (direction) {
		// Player movement
	case player_movement_en::positive_z:
		m_velocity += m_NY_cam_front * direction_mult;
		break;
	case player_movement_en::negative_z:
		m_velocity -= m_NY_cam_front * direction_mult;
		break;
	case player_movement_en::negative_x:
		m_velocity -= m_NY_cam_right * direction_mult;
		break;
	case player_movement_en::positive_x:
		m_velocity += m_NY_cam_right * direction_mult;
		break;
	// Flying movement
	case player_movement_en::flying_up:
		m_velocity.y += direction_mult;
		break;
	case player_movement_en::flying_down:
		m_velocity.y -= direction_mult;
		break;
	// Jumping movement
	case player_movement_en::jumping:
		if (player.grounded) m_velocity.y = 0.2f;
		break;
	default:
		break;
	}
}

int player_inst::find_slot_with_item(block_id item, bool include_full_slots) noexcept
{
	// Return index of item in inventory or -1 if none was found, taking 'fullness' into account
	constexpr int num_slots = static_cast<int>(sizeof(world_acc_player::inventory) / sizeof(world_acc_player::inv_slot_obj));
	constexpr int max_slot_count = 64;
	
	for (int i = 0; i < num_slots; ++i) { 
		const world_acc_player::inv_slot_obj *slot = player.inventory + i;
		if (slot->held_id != item) continue;
		if (include_full_slots && static_cast<int>(slot->count) >= max_slot_count) continue;
		return i;
	}

	return -1;
}

int player_inst::find_free_or_matching_slot(block_id item) noexcept
{
	// Returns the earliest air or non-full slot with the same id
	const int matching_slot_ind = find_slot_with_item(item, true);
	if (matching_slot_ind != -1) return matching_slot_ind;
	const int air_slot = find_slot_with_item(block_id::air, true);
	if (air_slot != -1) return air_slot;
	return -1;
}

// TODO: function order

void player_inst::draw_selected_outline() const noexcept
{
	if (!block_properties::of_block(player.target_block_id)->solid) return; // Only show outline of solid blocks
	game.shaders.programs.outline.bind_and_use(m_outline_vao); // Use correct buffers and shaders
	glDrawElements(GL_LINES, 48, GL_UNSIGNED_BYTE, nullptr); // Each pair of indexes determine the line's start and end position
}

void player_inst::draw_inventory_gui()  noexcept
{
	game.shaders.programs.inventory.bind_and_use(m_inventory_vao); // Bind VAO and use inventory shader
	// Draw hotbar instances (crosshair only if inventory is closed)
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, m_hotbar_instances - player.inventory_open);

	for (int i = 0; i < 9; ++i) ::glob_txt_rndr->render_single(m_hotbar_text + i, !i, true); // Render hotbar text
	if (!player.inventory_open) return; // Don't draw inventory elements if not open

	game.shaders.programs.inventory.bind_and_use(m_inventory_vao); // Switch back to inventory drawing
	// Draw rest of inventory (dark background, slots on the middle of the screen)
	glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, m_inv_instances - m_hotbar_instances, m_hotbar_instances);
	for (int i = 9; i < 36; ++i) ::glob_txt_rndr->render_single(m_slot_text + i, i == 9, true); // Render inventory text
}

void player_inst::update_cam_dir_vars(double x, double y) noexcept
{
	if (player.inventory_open) return;
	
	// Calculate camera angles
	player.yaw += x * game.rel_mouse_sens;
	player.yaw = player.yaw > 180.0 ? 
	  (-180.0 + (player.yaw - 180.0)) : 
	      (player.yaw < -180.0) ? 
	      (180.0 + (player.yaw + 180.0)) :
	      player.yaw;
	
	// Looking straight up causes camera to flip, so stop just before 90 degrees
	const double not_90 = 89.9999f;
	player.pitch = math::clamp(player.pitch + y * game.rel_mouse_sens, -not_90, not_90);
	player.moved = true; // Trigger update for other camera-related variables

	// Determine camera direction
	player.look_dir_pitch = player.pitch >= 0.0 ? wdir_up : wdir_down;
	player.look_dir_yaw =
		(player.yaw >= -45.0 && player.yaw <  45.00) ? wdir_front :
		(player.yaw >= 45.00 && player.yaw <  135.0) ? wdir_right :
		(player.yaw >= 135.0 || player.yaw < -135.0) ? wdir_back  : wdir_left;

	upd_cam_vectors(); // Update camera vectors
	upd_raycast_selected(); // Do raycasting to find the block the player is looking at
}

void player_inst::upd_raycast_selected() noexcept
{
	// DDA (Digital Differential Analyser) algorithm for raycasting:
	// Rather than moving in fixed steps and potentially missing a block, this algorithm
	// traverses in a grid-like pattern, ensuring all blocks in the way are detected.
	// See https://lodev.org/cgtutor/raycasting.html for the original source.

	// Determine steps in each axis
	vector3d ray_delta = { 
		math::abs(1.0 / m_cam_front.x), 
		math::abs(1.0 / m_cam_front.y), 
		math::abs(1.0 / m_cam_front.z) 
	};

	// Store data about block player was previously looking at
	const world_pos org_selected_pos = player.selected_block_pos;
	const block_id org_selected_block = player.target_block_id;

	// Initial values for raycasting
	player.selected_block_pos = chunk_vals::to_block_pos(&player.position);
	vector3d side_dist;
	vector3i step;

	// Determine directions and magnitude in each axis
	// TODO: equation
	
	// X axis
	if (m_cam_front.x < 0.0) {
		step.x = -1;
		side_dist.x = (player.position.x - static_cast<double>(player.selected_block_pos.x)) * ray_delta.x;
	} else {
		step.x = 1;
		side_dist.x = (static_cast<double>(player.selected_block_pos.x) + 1.0 - player.position.x) * ray_delta.x;
	}
	// Y axis
	if (m_cam_front.y < 0.0) {
		step.y = -1;
		side_dist.y = (player.position.y - static_cast<double>(player.selected_block_pos.y)) * ray_delta.y;
	} else {
		step.y = 1;
		side_dist.y = (static_cast<double>(player.selected_block_pos.y) + 1.0 - player.position.y) * ray_delta.y;
	}
	// Z axis
	if (m_cam_front.z < 0.0) {
		step.z = -1;
		side_dist.z = (player.position.z - static_cast<double>(player.selected_block_pos.z)) * ray_delta.z;
	} else {
		step.z = 1;
		side_dist.z = (static_cast<double>(player.selected_block_pos.z) + 1.0 - player.position.z) * ray_delta.z;
	}

	// Used to calculate where the 'side' of the block is depending on where the player looks at the block
	world_pos prev_target_loc = -chunk_vals::dirs_xyz[player.look_dir_yaw]; // Default value

	// Update block ID of ray block and return if it is solid (stop raycasting if it is)
	const auto check_curr_block = [&]{
		// Determine block at current raycast position
		player.target_block_id = world->block_at(&player.selected_block_pos);
		// Stop if a solid block is in the way
		return block_properties::of_block(player.target_block_id)->solid;
	};
	
	int blocks_reach_dist = 7; // Counter for how many blocks out the player can reach
	
	// If the player is already inside of a block, don't bother raycasting (1 due to pre-increment check)
	if (check_curr_block()) blocks_reach_dist = 1;

	while (--blocks_reach_dist) {
		prev_target_loc = player.selected_block_pos;
		// Check shortest axis and move in that direction
		enum class raycast_axis { RC_x, RC_y, RC_z } shortest_axis = 
		    side_dist.x < side_dist.y ? 
		   (side_dist.x < side_dist.z ? raycast_axis::RC_x : raycast_axis::RC_z) :
		   (side_dist.y < side_dist.z ? raycast_axis::RC_y : raycast_axis::RC_z);
		
		switch (shortest_axis)
		{
		case raycast_axis::RC_x:
			player.selected_block_pos.x += step.x;
			side_dist.x += ray_delta.x;
			break;
		case raycast_axis::RC_y:
			player.selected_block_pos.y += step.y;
			side_dist.y += ray_delta.y;
			break;
		case raycast_axis::RC_z:
			player.selected_block_pos.z += step.z;
			side_dist.z += ray_delta.z;
			break;
		default:
			break;
		}

		if (check_curr_block()) break; // Check what block is at the current ray position and stop if it is solid
	}

	// Use to determine the 'side' the player is looking at for block placement
	place_rel_pos = prev_target_loc - player.selected_block_pos;

	// Update block text if it's a different block than the previous (differs by position or type)
	if (player.target_block_id != org_selected_block || player.selected_block_pos != org_selected_pos) {
		update_selected_block_info_txt();
	}
}

void player_inst::update_selected_block_info_txt() noexcept
{
	// Get block properties of selected (looking at) block
	const block_properties::block_attributes *block_data = block_properties::of_block(player.target_block_id);

	// Format string with block information text
	const world_pos offset = chunk_vals::world_to_offset(&player.selected_block_pos);
	const std::string ray_block_info_text = formatter::fmt(
		"Selected:\n"
		 VXF64 " " VXF64" " VXF64 " (" VXF64 " " VXF64 " " VXF64 ")\n"
		"\"%s\" (ID %d)\n"
		"Light emission: %d\n"
		"Strength: %d\n"
		"Solid: %d\n"
		"Transparency: %d\n"
		"Textures: %d %d %d %d %d %d", 
		player.selected_block_pos.x, player.selected_block_pos.y, player.selected_block_pos.z,
		offset.x, offset.y, offset.z,
		block_data->name, static_cast<int>(block_data->mesh_info.id),
		block_data->emission,
		block_data->strength,
		block_data->solid,
		block_data->mesh_info.has_trnsp,
		block_data->textures[0], block_data->textures[1], block_data->textures[2],
		block_data->textures[3], block_data->textures[4], block_data->textures[5]
	);

	m_block_text.set_text(ray_block_info_text); // Update text with block information
	m_block_text.set_pos({ 0.99f - m_block_text.get_width(), 0.99f }); // Update position using width
}

void player_inst::update_plr_offset() noexcept
{
	const world_pos initial = player.offset; // Get initial offset for comparison
	player.offset = chunk_vals::world_to_offset(&player.position); // Calculate new player offset
	if (!game.do_generate_signal || (initial.x == player.offset.x && initial.z == player.offset.z)) return;
	world->signal_generation_thread(); // Update chunks (generation, deletion, etc) if offset changed
}

void player_inst::upd_cam_vectors() noexcept
{
	// Trig values from yaw and pitch radians
	const double yaw_radians = math::radians(player.yaw);
	const double pitch_radians = math::radians(player.pitch);
	const double pitch_radians_cosine = ::cos(pitch_radians);
	
	// Set front and right camera vectors
	m_cam_front = vector3d(
		::cos(yaw_radians) * pitch_radians_cosine,
		::sin(pitch_radians),
		::sin(yaw_radians) * pitch_radians_cosine
	).to_unit();
	m_cam_right = m_cam_front.c_cross({ 0.0, 1.0, 0.0 }).to_unit();

	// Calculate up vector using the two
	m_cam_up = m_cam_right.c_cross(m_cam_front).to_unit();

	// Set the front and right vectors excluding Y axis/pitch for movement ignoring camera pitch
	m_NY_cam_front = vector3d(m_cam_front.x, 0.0, m_cam_front.z).to_unit();
	m_NY_cam_right = vector3d(m_cam_right.x, 0.0, m_cam_right.z).to_unit();

	update_frustum_vals(); // Update camera frustum to use new camera vectors
}

void player_inst::upd_inv_scroll_relative(float scroll_y_amnt) noexcept
{
	m_selected = static_cast<uint8_t>((static_cast<int>(m_selected) + math::sign(scroll_y_amnt) + 9) % 9);	
	update_inventory_buffers();
}

void player_inst::upd_inv_selected(int new_selected_slot_ind) noexcept
{
	m_selected = static_cast<uint8_t>(new_selected_slot_ind);
	update_inventory_buffers();
}

vector2f player_inst::get_slot_pos(slot_type_en stype, int id, bool is_block_tex) const noexcept
{
	// Inventory elements sizes and positions
	const float slot_width = 0.1f, slot_height = slot_width * game.window_wh_aspect;
	const float slot_blocktex_width = slot_width * 0.75f, slot_blocktex_height = slot_height * 0.75f;
	const float slot_blocktex_y_offset = (slot_height - slot_blocktex_height) * 0.5f;
	constexpr float slots_start_xpos = -0.45f;
	constexpr float inventory_hotbar_ypos = -0.4f;

	float y_pos; // Resulting y position (x position is defined at end - simpler to calculate)

	switch (stype)
	{
	case slot_type_en::hotbar:
		y_pos = -1.0f;
		break;
	case slot_type_en::inventory:
		y_pos = inventory_hotbar_ypos;
		if (id >= 9) y_pos += (slot_height * 1.1f) + (static_cast<float>((id - 9) / 9) * slot_height * 0.98f);
		break;
	case slot_type_en::slot_size:
		return { slot_width, slot_height };
	case slot_type_en::slot_block_size:
		return { slot_blocktex_width, slot_blocktex_height };
	default:
		y_pos = 0.0f;
		break;
	}

	// Resulting x position
	const float x_pos = slots_start_xpos + (static_cast<float>(id % 9) * slot_width * 0.98f);
	
	// Slot block textures are slightly offset from normal inventory slots
	if (is_block_tex) return { x_pos + ((slot_width - slot_blocktex_width) * 0.5f), y_pos + slot_blocktex_y_offset };
	else return { x_pos, y_pos };
}

vector4f player_inst::get_slot_dims(slot_type_en stype, int id, bool is_block_tex) const noexcept
{
	return vector4f(
		get_slot_pos(stype, id, is_block_tex),
		get_slot_pos(is_block_tex ? slot_type_en::slot_block_size : slot_type_en::slot_size, 0, false)
	);
}

void player_inst::update_inventory_buffers() noexcept
{
	// Each texture index (when data is of an inventory texture rather than a block texture)
	// is used to index into an array that determines the texture positions in the vertex shader.
	enum tex_ind_en : uint32_t { TI_background, TI_unequipped, TI_equipped, TI_crosshair };
	m_inv_instances = 0; // Reset inventory instances count
	
	const auto add_slot_instance = [&](int inv_id, slot_type_en slot_type, tex_ind_en texture) noexcept {
		const auto add_inst = [&](uint32_t tex, bool is_block_tex) noexcept {
			inventory_inst new_inv_inst = { get_slot_dims(slot_type, inv_id, is_block_tex), tex, is_block_tex };
			inventory_data[m_inv_instances++] = new_inv_inst;
			if (inv_id >= 9) return; // Only copy inventory slots
			// Copy of hotbar instances in inventory, store at back for now to determine index after hotbar and such
			new_inv_inst.dims.y = get_slot_pos(slot_type_en::inventory, inv_id, is_block_tex).y;
			inventory_data[inventory_elems_count - m_inv_instances] = new_inv_inst;
		};

		add_inst(texture, false); // Add normal inventory instance
		const block_id item_slot_id = player.inventory[inv_id].held_id; // Add block instance if there is a non-air item
		if (item_slot_id != block_id::air) add_inst(block_properties::of_block(item_slot_id)->textures[wdir_up], true);
	};

	// Hotbar slots calculation, make selected slot render on top by being last
	for (int hotbar_id = 0; hotbar_id < 9; ++hotbar_id) if (hotbar_id != m_selected) add_slot_instance(hotbar_id, slot_type_en::hotbar, TI_unequipped);
	add_slot_instance(m_selected, slot_type_en::hotbar, TI_equipped);

	const float ch_size = 0.025f, ch_width = ch_size / game.window_wh_aspect; // Crosshair at the center of the screen
	inventory_data[m_inv_instances++] = { { -ch_width, -ch_size, ch_width, ch_size }, TI_crosshair, false };

	m_hotbar_instances = m_inv_instances; // Use only previous elements when inventory is off (hotbar instances + crosshair)

	inventory_data[m_inv_instances++] = { { -1.0f, -1.0f, 2.0f, 2.0f }, TI_background, false}; // Fullscreen inventory GUI background

	// Add inventory hotbar section, stored at the back of the inventory instances array
	for (int i = 1; i <= m_hotbar_instances - 1; ++i) inventory_data[m_inv_instances++] = inventory_data[inventory_elems_count - i];
	// Do same as hotbar but with normal inventory (starts at inv. index 9 to skip hotbar)
	for (int inv_id = 9; inv_id < 36; ++inv_id) add_slot_instance(inv_id, slot_type_en::inventory, TI_unequipped);

	// Bind VAO and VBO to ensure the correct buffers are edited
	glBindVertexArray(m_inventory_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_inventory_inst_vbo);
	// Buffer inventory data for use in the shader - only include the filled data rather than the whole array
	glBufferData(GL_ARRAY_BUFFER, sizeof(inventory_inst) * m_inv_instances, inventory_data, GL_DYNAMIC_DRAW);
}

void player_inst::update_item_text_pos() noexcept
{
	const vector2f text_offset = { 0.02f, 0.045f * game.window_wh_aspect };
	for (int i = 0; i < 36; ++i) m_slot_text[i].set_pos(get_slot_pos(slot_type_en::inventory, i, false) + text_offset);
	for (int i = 0; i < 9; ++i) m_hotbar_text[i].set_pos(get_slot_pos(slot_type_en::hotbar, i, false) + text_offset);
}

void player_inst::update_slot(int slot_ind, block_id b_id, uint8_t count)
{
	world_acc_player::inv_slot_obj *const slot = player.inventory + slot_ind;
	text_renderer_obj::text_obj *const slot_text = m_slot_text + slot_ind;
	bool needs_slot_upd = b_id != slot->held_id; // Determine if inventory elements need to be updated
	const bool mirror_to_hotbar = slot_ind < 9; // Some inventory slots are copied to the hotbar

	// Update text and slot data with arguments, hiding it if the new count is 0 or the new object is air
	if (count == 0 || b_id == block_id::air) {
		// Zero count or air object - reset slot data and hide counter
		slot_text->set_visibility(false);
		slot->held_id = block_id::air;
		slot->count = 0;

		needs_slot_upd = true; // Update block texture to none for air
		if (mirror_to_hotbar) m_hotbar_text[slot_ind].set_visibility(false);
	} else {
		const std::string new_text = std::to_string(count);
		// Positive count and non-air object - update text and slot data
		slot->held_id = b_id;
		slot->count = count;
		
		slot_text->set_text(new_text);
		slot_text->set_visibility(true);

		if (mirror_to_hotbar) {
			text_renderer_obj::text_obj *const hotbar_text = m_hotbar_text + slot_ind;
			hotbar_text->set_text(new_text);
			hotbar_text->set_visibility(true);
		}
	}

	// Update inventory GUI to display new block texture - do after slot calculation to use new values
	if (needs_slot_upd) update_inventory_buffers();
}

player_inst::~player_inst()
{
	// List of all buffers from outline and inventory to be deleted
	const GLuint delete_bufs[] = { m_outline_vbo, m_inventory_inst_vbo, m_inventory_quad_vbo };

	// Delete the buffers from the above array
	glDeleteBuffers(static_cast<GLsizei>(math::size(delete_bufs)), delete_bufs);

	// Delete both VAOs
	const GLuint delete_vaos[] = { m_outline_vao, m_inventory_vao };
	glDeleteVertexArrays(static_cast<GLsizei>(math::size(delete_vaos)), delete_vaos);

	delete[] inventory_data; // Delete inventory instances pointer
}
