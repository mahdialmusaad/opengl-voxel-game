#include "player/camera.h"
#include "player/movement.h"
#include "player/raycast.h"
#include "player/frustum.h"

#include "directives/dcast.h"
#include "directives/dmath.h"

#include "values/elements.h"
#include "values/state.h"

#include "vector/vec3.h"
#include "vector/mat4.h"

#include <math.h>

#define VX_UNUSED(x) do { (void)(x); } while (0)

struct vxplr_cam_directions_obj vxplr_cam = {
	{ 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, /* Front, right and up vectors. */
	{ 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, /* Front and right vectors without Y. */
	0.0, 0.0, 0.0, /* Pitch, yaw and roll. */
	90.0 * VX_RADIAN_MULT, 0.1, 0.4, /* FOV, sensitivity and zoom. */
	0, 0, /* Direction index of pitch and yaw. */
	/* Frustum object. */
	{{
		{ { 0.0, 0.0, 0.0 }, 0.0 }, { { 0.0, 0.0, 0.0 }, 0.0 }, { { 0.0, 0.0, 0.0 }, 0.0 },
		{ { 0.0, 0.0, 0.0 }, 0.0 }, { { 0.0, 0.0, 0.0 }, 0.0 },
	}}
};

static inline double vxplr_yaw_loop(double yaw)
{
	if (yaw > 180.0) return -360.0 + yaw;
	else if (yaw < -180.0) return 360.0 + yaw;
	else return yaw;
}

void vxplr_cam_mouse_move(double x, double y, double delta_x, double delta_y)
{
	VX_UNUSED(x); VX_UNUSED(y);
	if (vxtg_toggles.inventory_open) return;

	vxplr_cam.yaw += delta_x * vxplr_cam.sensitivity;
	vxplr_cam.yaw = vxplr_yaw_loop(vxplr_cam.yaw);

	/* Looking straight up causes camera to flip due to matrix, so stop right before. */
	const double not_right_angle = 89.99;
	const double frame_pitch = vxplr_cam.pitch + (delta_y * vxplr_cam.sensitivity);
	vxplr_cam.pitch = VX_CLAMP(frame_pitch, -not_right_angle, not_right_angle);

	vxplr_cam_dirs_update();
}

void vxplr_cam_dirs_update(VX_NO_ARG)
{
	vxplr_cam.pitch_dir = vxplr_cam.pitch >= 0.0 ? wdir_top : wdir_down;
	vxplr_cam.yaw_dir =
		(vxplr_cam.yaw >= -45.0 && vxplr_cam.yaw <   45.0) ? wdir_front :
		(vxplr_cam.yaw >=  45.0 && vxplr_cam.yaw <  135.0) ? wdir_right :
		(vxplr_cam.yaw >= 135.0 || vxplr_cam.yaw < -135.0) ? wdir_back  : wdir_left;

	const double yaw_radians = VX_RADIAN_MULT * vxplr_cam.yaw;
	const double pitch_radians = VX_RADIAN_MULT * vxplr_cam.pitch;
	const double pitch_radians_cosine = cos(pitch_radians);

	vxplr_cam.fly_front.x = vxplr_cam.front.x = cos(yaw_radians) * pitch_radians_cosine;
	vxplr_cam.front.y = sin(pitch_radians);
	vxplr_cam.fly_front.z = vxplr_cam.front.z = sin(yaw_radians) * pitch_radians_cosine;

	dvec3_unit(&vxplr_cam.front, &vxplr_cam.front);
	dvec3_unit(&vxplr_cam.fly_front, &vxplr_cam.fly_front);

	const dvec3 world_up = { 0.0, 1.0, 0.0 };
	dvec3_cross(&vxplr_cam.fly_right, &vxplr_cam.fly_front, &world_up);
	dvec3_cross(&vxplr_cam.right, &vxplr_cam.front, &world_up);
	dvec3_cross(&vxplr_cam.up, &vxplr_cam.right, &vxplr_cam.front);

	dvec3_unit(&vxplr_cam.right, &vxplr_cam.right);
	dvec3_unit(&vxplr_cam.up, &vxplr_cam.up);

	vxplr_cam_update_frustum();
	vxplr_ray_cast_local(&vxplr_inst.pos, &vxplr_cam.front);
}

mat4 *vxplr_cam_origin_matrix(mat4 *res)
{
	const vec3 zero_pos = { 0.0f, 0.0f, 0.0f };
	const vec3 front_floating = {
		VX_CAST(float, vxplr_cam.front.x),
		VX_CAST(float, vxplr_cam.front.y),
		VX_CAST(float, vxplr_cam.front.z)
	};
	const vec3 up_floating = {
		VX_CAST(float, vxplr_cam.up.x),
		VX_CAST(float, vxplr_cam.up.y),
		VX_CAST(float, vxplr_cam.up.z)
	};

	mat4_look(res, &zero_pos, &front_floating, &up_floating);
	return res;
}
mat4 *vxplr_cam_matrix(mat4 *res)
{
	const vec3 pos_floating = {
		VX_CAST(float, vxplr_inst.pos.x),
		VX_CAST(float, vxplr_inst.pos.y),
		VX_CAST(float, vxplr_inst.pos.z)
	};
	const vec3 front_floating = {
		VX_CAST(float, vxplr_inst.pos.x) + VX_CAST(float, vxplr_cam.front.x),
		VX_CAST(float, vxplr_inst.pos.y) + VX_CAST(float, vxplr_cam.front.y),
		VX_CAST(float, vxplr_inst.pos.z) + VX_CAST(float, vxplr_cam.front.z)
	};
	const vec3 up_floating = {
		VX_CAST(float, vxplr_cam.up.x),
		VX_CAST(float, vxplr_cam.up.y),
		VX_CAST(float, vxplr_cam.up.z)
	};

	mat4_look(res, &pos_floating, &front_floating, &up_floating);
	return res;
}


void vxplr_cam_update_fov(double degrees)
{
	degrees = VX_CLAMP(degrees, VX_CAMERA_FOV_MIN, VX_CAMERA_FOV_MAX);
	vxplr_cam.fov = degrees * VX_RADIAN_MULT;
	vxplr_cam_update_frustum();
}

void vxplr_cam_update_frustum(VX_NO_ARG)
{
	vxfrs_update_frustum(
		&vxplr_cam.frustum,
		&vxplr_inst.pos,
		&vxplr_cam.front,
		&vxplr_cam.up,
		&vxplr_cam.right,
		vxplr_cam.fov,
		VX_CAST(double, vxstate_vals.aspect)
	);
}
