#pragma once
#ifndef SOURCE_PLAYER_MOVEMENT_VXL_HDR
#define SOURCE_PLAYER_MOVEMENT_VXL_HDR
/* Player positioning values. */

#include "directives/dextern.h"

#include "vector/wpos.h"
#include "vector/vec3.h"

struct vxstruct_plr_instance
{
	/* Current camera position. */
	dvec3 pos;
	/* Current player velocity. */
	dvec3 vel;
	/* Current camera position chunk offset. */
	wpos offset;
	
	/* Speed values. */
	double base_speed, active_speed, run_multiplier, slow_multiplier;
	/* Other values. */
	double jump_height, terminal, gravity;

	/* Detected block at the player's head. */
	unsigned short head_block;
	/* Detected ground block. */
	unsigned short feet_block;

	int free;
};
/* Player position and movement values. */
extern struct vxstruct_plr_instance vxplr_inst;

VX_C_START

/* Movement logic - check held keys, change velocity, collision. */
void vxplr_move_logic(VX_NO_ARG);
/* Set the player's position. */
void vxplr_move_set_position(const dvec3 *set_pos);

VX_C_END

#endif
