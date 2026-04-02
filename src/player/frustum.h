#pragma once
#ifndef SOURCE_RENDERING_FRUSTUM_VXL_HDR
#define SOURCE_RENDERING_FRUSTUM_VXL_HDR
/* Frustum culling functions. */

#include "directives/dextern.h"
#include "directives/dword.h"

#include "vector/vec3.h"

#define VX_FRUSTUM_NEAR (0.05)
#define VX_FRUSTUM_FAR (10000.0)

/* A frustum plane. */
struct vxfrs_plane_obj
{
	dvec3 normal;
	double distance;
};

enum
{
	vxen_frustum_near = 0,
	vxen_frustum_right = 1,
	vxen_frustum_left = 2,
	vxen_frustum_low = 3,
	vxen_frustum_top = 4,
};

/* Full frustum object. */
struct vxfrs_frustum_obj
{
	struct vxfrs_plane_obj planes[5];
};


VX_C_START

struct dvec3;

/* Update frustum values. */
void vxfrs_update_frustum(
	struct vxfrs_frustum_obj *VX_RESTRICT frustum,
	const struct dvec3 *VX_RESTRICT position,
	const struct dvec3 *VX_RESTRICT front,
	const struct dvec3 *VX_RESTRICT up,
	const struct dvec3 *VX_RESTRICT right,
	double fov_y,
	double aspect
);

/* Returns whether a sphere with a given position and radius would be visible in the given frustum. */
int vxfrs_sphere_visible(
	struct vxfrs_frustum_obj *VX_RESTRICT frustum,
	const struct dvec3 *VX_RESTRICT center,
	double radius
);
/* Returns whether a given cuboid would be visible in the given frustum. */
int vxfrs_cuboid_visible(
	struct vxfrs_frustum_obj *VX_RESTRICT frustum,
	const struct dvec3 *VX_RESTRICT neg_corner,
	double xtotal, double ytotal, double ztotal
);

VX_C_END

#endif
