#pragma once
#ifndef _SOURCE_PLAYER_PLRDEF_HDR_
#define _SOURCE_PLAYER_PLRDEF_HDR_

#include "World/Chunk.hpp"
#include "Rendering/Frustum.hpp"

// Player variables that are able to be accessed by the world
struct world_acc_player
{
	struct inv_slot_obj
	{
		block_id held_id = block_id::air;
		uint8_t count = 0;
	};

	bool ignore_collision = false;
	bool gravity_effects = false;
	bool inventory_open = false;

	bool moved = false;
	bool grounded = true;
	
	block_id target_block_id = block_id::air;
	block_id head_block   = block_id::air;
	block_id feet_block   = block_id::air;
	
	int8_t look_dir_pitch = wdir_none, look_dir_yaw = wdir_none;
	
	vector3d position{ 16.5, chunk_vals::water_y + 10, 16.5 };
	
	world_pos offset{};
	camera_frustum frustum{};

	inv_slot_obj inventory[36]{};
	world_pos selected_block_pos{};
	
	double fov = math::radians(90.0);
	double pitch = 0.0, yaw = 0.0;
	double base_speed = 1.0, active_speed = 1.0;
};

#endif // _SOURCE_PLAYER_PLRDEF_HDR_
