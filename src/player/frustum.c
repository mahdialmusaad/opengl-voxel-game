#include "player/frustum.h"

#include "vector/vec3.h"

static void vxfrs_update_plane(
	struct vxfrs_plane_obj *VX_RESTRICT plane,
	const dvec3 *VX_RESTRICT dist_vec,
	const dvec3 *VX_RESTRICT to_norm
) {
	dvec3_unit(&plane->normal, to_norm);
	plane->distance = dvec3_dot(to_norm, dist_vec);
}

void vxfrs_update_frustum(
	struct vxfrs_frustum_obj *VX_RESTRICT frustum,
	const dvec3 *VX_RESTRICT position,
	const dvec3 *VX_RESTRICT cam_front,
	const dvec3 *VX_RESTRICT cam_up,
	const dvec3 *VX_RESTRICT cam_right,
	double fov_y,
	double aspect
) {
	const double half_v = VX_FRUSTUM_FAR * tan(fov_y * 0.5);
	const double half_h = half_v * aspect;

	/* Frustum values setup. */
	dvec3 front_mul_far, right_mul_hh, up_mul_hv, tmp_calc, tmp_result;
	dvec3_mul(&front_mul_far, cam_front, VX_FRUSTUM_FAR);
	dvec3_mul(&right_mul_hh, cam_right, half_h);
	dvec3_mul(&up_mul_hv, cam_up, half_v);

	dvec3_mul(&tmp_calc, cam_front, VX_FRUSTUM_NEAR);
	dvec3_add(&tmp_result, &tmp_calc, position);
	vxfrs_update_plane(&frustum->planes[vxen_frustum_near], &tmp_result, cam_front);

	dvec3_sub(&tmp_calc, &front_mul_far, &right_mul_hh);
	dvec3_cross(&tmp_result, &tmp_calc, cam_up);
	vxfrs_update_plane(&frustum->planes[vxen_frustum_right], position, &tmp_result);

	dvec3_add(&tmp_calc, &front_mul_far, &right_mul_hh);
	dvec3_cross(&tmp_result, cam_up, &tmp_calc);
	vxfrs_update_plane(&frustum->planes[vxen_frustum_left], position, &tmp_result);

	dvec3_sub(&tmp_calc, &front_mul_far, &up_mul_hv);
	dvec3_cross(&tmp_result, cam_right, &tmp_calc);
	vxfrs_update_plane(&frustum->planes[vxen_frustum_top], position, &tmp_result);

	dvec3_add(&tmp_calc, &front_mul_far, &up_mul_hv);
	dvec3_cross(&tmp_result, &tmp_calc, cam_right);
	vxfrs_update_plane(&frustum->planes[vxen_frustum_low], position, &tmp_result);
}

int vxfrs_sphere_visible(
	struct vxfrs_frustum_obj *VX_RESTRICT frustum,
	const dvec3 *VX_RESTRICT center,
	double radius
) {
	return
	   frustum->planes[0].distance - dvec3_dot(&frustum->planes[0].normal, center) <= radius &&
	   frustum->planes[1].distance - dvec3_dot(&frustum->planes[1].normal, center) <= radius &&
	   frustum->planes[2].distance - dvec3_dot(&frustum->planes[2].normal, center) <= radius &&
	   frustum->planes[3].distance - dvec3_dot(&frustum->planes[3].normal, center) <= radius &&
	   frustum->planes[4].distance - dvec3_dot(&frustum->planes[4].normal, center) <= radius;
}

int vxfrs_cuboid_visible(
	struct vxfrs_frustum_obj *VX_RESTRICT frustum,
	const struct dvec3 *VX_RESTRICT neg_corner,
	double xtotal, double ytotal, double ztotal
) {
	for (int i = 0; i < 5; ++i) {
		struct vxfrs_plane_obj *plane = frustum->planes + i;
		const dvec3 vertex = {
			neg_corner->x + ((plane->normal.x >= 0) * xtotal),
			neg_corner->y + ((plane->normal.y >= 0) * ytotal),
			neg_corner->z + ((plane->normal.z >= 0) * ztotal)
		};
		if (plane->distance + dvec3_dot(&plane->normal, &vertex) < 0) return 0;
	}
	return 1;
}
