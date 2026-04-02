#include "player/movement.h"
#include "player/raycast.h"
#include "player/camera.h"

#include "values/state.h"

#include "graphics/glfw.h"

#include "events/events.h"
#include "events/keys.h"

#include "world/modify.h"
#include "world/locate.h"

#include "vector/wpos.h"
#include "vector/vec3.h"

#include <math.h>

struct vxstruct_plr_instance vxplr_inst = {
	{ VX_WLD_CHUNK_XBLKS / 2.0, VX_WLD_CHUNK_YBLKS / 2.0, VX_WLD_CHUNK_ZBLKS / 2.0 }, { 0.0, 0.0, 0.0 }, { 0, 0, 0 }, /* Position-related values. */
	20.0, 0.0, 2.0, 0.5, /* Speed values (base, active, run mult, slow mult). */
	0.2, -10.0, -0.9, /* Other values (jump, terminal, gravity). */
	0, 0, /* Relevant block IDs. */
	0 /* Free space for later use. */
};

/* Update values that depend on the player position. */
static void vxplr_move_position_update(VX_NO_ARG)
{
	wpos plr_block_pos;
	vxwld_global_integral_position(&vxplr_inst.pos, &plr_block_pos);

	vxwld_get(&plr_block_pos);
	const wpos feet_block_pos = { plr_block_pos.x, plr_block_pos.y - 2, plr_block_pos.z };
	vxwld_get(&feet_block_pos);

	vxwld_global_position_offset(&plr_block_pos, &vxplr_inst.offset);
	vxplr_cam_update_frustum();
	vxplr_ray_cast_local(&vxplr_inst.pos, &vxplr_cam.front);

	VX_EVENTS_HOOK_EXECUTE(player_moved,);
}

static void vxplr_apply_velocity(VX_NO_ARG)
{
	dvec3_add(&vxplr_inst.pos, &vxplr_inst.pos, &vxplr_inst.vel);
	/* Ignore very small velocities that have practically no impact. */
	if (dvec3_dot(&vxplr_inst.vel, &vxplr_inst.vel) < 0.001) return;

	vxplr_move_position_update();
}

static double vxplr_decline_speed(double current)
{
	const double dt = vxstate_vals.frame_delta;
	return current * dt;
}

void vxplr_move_logic(VX_NO_ARG)
{
	/* Gradually slow down the player in each axis. */
	vxplr_inst.vel.x = vxplr_decline_speed(vxplr_inst.vel.x);
	vxplr_inst.vel.z = vxplr_decline_speed(vxplr_inst.vel.z);
	vxplr_inst.vel.y = vxtg_toggles.gravity ? 
		fmax(vxplr_inst.vel.y + (vxplr_inst.gravity * vxstate_vals.frame_delta), vxplr_inst.terminal) :
		vxplr_decline_speed(vxplr_inst.vel.y);

	/* Not possible to move using keys, ignore input checks. */
	if (vxtg_toggles.chatting || vxtg_toggles.inventory_open) {
		vxplr_apply_velocity();
		return;
	}

	/* Speed up or slow down depending on held speed modifier key. */
	const double target_multiplier =
		vxkey_states[GLFW_KEY_LEFT_SHIFT] ? vxplr_inst.run_multiplier :
		(vxkey_states[GLFW_KEY_LEFT_ALT] ? vxplr_inst.slow_multiplier : 1.0);
	vxplr_inst.active_speed = vxplr_inst.base_speed * target_multiplier;

	const double direction_mult = vxplr_inst.active_speed * vxstate_vals.frame_delta;
	const double org_y = vxplr_inst.vel.y;
	dvec3 intermediate;

	const dvec3 *target_front = vxtg_toggles.gravity ? &vxplr_cam.front : &vxplr_cam.fly_front;
	const dvec3 *target_right = vxtg_toggles.gravity ? &vxplr_cam.right : &vxplr_cam.fly_right;

	/* Check for specific direction input per frame. */
	if (vxkey_states[GLFW_KEY_W] | vxkey_states[GLFW_KEY_UP]) {
		dvec3_mul(&intermediate, target_front, direction_mult);
		dvec3_add(&vxplr_inst.vel, &vxplr_inst.vel, &intermediate);
	}
	if (vxkey_states[GLFW_KEY_S] | vxkey_states[GLFW_KEY_DOWN]) {
		dvec3_mul(&intermediate, target_front, direction_mult);
		dvec3_sub(&vxplr_inst.vel, &vxplr_inst.vel, &intermediate);
	}
	if (vxkey_states[GLFW_KEY_A] | vxkey_states[GLFW_KEY_LEFT]) {
		dvec3_mul(&intermediate, target_right, direction_mult);
		dvec3_sub(&vxplr_inst.vel, &vxplr_inst.vel, &intermediate);
	}
	if (vxkey_states[GLFW_KEY_D] | vxkey_states[GLFW_KEY_RIGHT]) {
		dvec3_mul(&intermediate, target_right, direction_mult);
		dvec3_add(&vxplr_inst.vel, &vxplr_inst.vel, &intermediate);
	}

	vxplr_inst.vel.y = org_y;

	/* Flying and jumping movement. */
	if (vxtg_toggles.gravity) {
		if (vxkey_states[GLFW_KEY_SPACE] && vxtg_toggles.grounded) vxplr_inst.vel.y = vxplr_inst.jump_height;
	} else {
		if (vxkey_states[GLFW_KEY_SPACE]) vxplr_inst.vel.y = org_y + direction_mult;
		else if (vxkey_states[GLFW_KEY_LEFT_CONTROL]) vxplr_inst.vel.y = org_y - direction_mult;
	}

	vxplr_apply_velocity();
}

void vxplr_move_set_position(const dvec3 *set_pos)
{
	vxplr_inst.pos = *set_pos;
	vxplr_move_position_update();
}
