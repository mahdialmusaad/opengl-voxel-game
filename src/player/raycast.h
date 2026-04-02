#pragma once
#ifndef SOURCE_PLAYER_RAYCAST_VXL_HDR
#define SOURCE_PLAYER_RAYCAST_VXL_HDR
/* Raycasting and block selection functions. */

#include "directives/dextern.h"

#include "vector/wpos.h"

/* Maximum distance to search for blocks in raycast. */
#define VX_RAY_MAX_ITERATIONS (7)

struct vxstruct_ray_state_obj
{
	/* The currently selected block's position. */
	wpos slctd_pos;
	/* World direction index of the 'side' of the selected block. */
	int slctd_side;
	/* Whether the player is inside a block when trying to raycast. */
	short ray_inside;
	/* The currently selected block's type ID. */
	unsigned short slctd_type;
};
/* Current values related to block selection and raycasting. */
extern struct vxstruct_ray_state_obj vxplr_ray;
struct dvec3;

VX_C_START

/* Initialize values for ray rendering. */
void vxplr_ray_init(VX_NO_ARG);

/* Cast a ray to determine what block is in the way. */
int vxplr_ray_cast(const struct dvec3 *start_pos, const struct dvec3 *direction, wpos *target_pos, int *target_normal_index, unsigned short *target_type);
/* Cast a ray for the player. */
void vxplr_ray_cast_local(const struct dvec3 *start_pos, const struct dvec3 *direction);

/* Force an update for the ray text. */
void vxplr_ray_update(VX_NO_ARG);

VX_C_END

#endif
