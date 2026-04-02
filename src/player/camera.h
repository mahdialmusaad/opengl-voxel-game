#pragma once
#ifndef SOURCE_PLAYER_CAMERA_VXL_HDR
#define SOURCE_PLAYER_CAMERA_VXL_HDR
/* Player camera values. */

#include "directives/dextern.h"

#include "player/frustum.h"

#include "vector/vec3.h"

/* Player camera directions values object. */
struct vxplr_cam_directions_obj
{
	/* Camera normal direction vectors. */
	dvec3 front, right, up;
	/* Flying direction vectors. */
	dvec3 fly_front, fly_right;
	/* Camera rotation directions. */
	double pitch, yaw, roll;
	/* Camera view angle in radians. */
	double fov;
	/* Camera movement sensitivity. */
	double sensitivity;
	/* FOV zoom intensity. */
	double zoom_mult;
	/* Direction enumeration representations of rotation. */
	int pitch_dir, yaw_dir;
	/* Player view frustum. */
	struct vxfrs_frustum_obj frustum;
};
/* Player camera directions values. */
extern struct vxplr_cam_directions_obj vxplr_cam;

#define VX_CAMERA_FOV_MIN (20.0)
#define VX_CAMERA_FOV_MAX (120.0)
struct mat4;

VX_C_START

/* Get a matrix that describes the camera rotation but positioned at the origin. */
struct mat4 *vxplr_cam_origin_matrix(struct mat4 *res);
/* Get a matrix that describes the camera. */
struct mat4 *vxplr_cam_matrix(struct mat4 *res);

/* Update the FOV to the specified degrees. */
void vxplr_cam_update_fov(double degrees);
/* Update the frustum values. */
void vxplr_cam_update_frustum(VX_NO_ARG);
/* Update camera direction values. */
void vxplr_cam_dirs_update(VX_NO_ARG);
/* Update camera direction values depending on current mouse delta. */
void vxplr_cam_mouse_move(double x, double y, double delta_x, double delta_y);

VX_C_END

#endif
