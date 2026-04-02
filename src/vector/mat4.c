#include "vector/mat4.h"
#include "vector/vec3.h"

#include "directives/dcast.h"

#include <string.h>

#include <math.h>

void mat4_mul(mat4 *VX_RESTRICT res, const mat4 *VX_RESTRICT lhs, const mat4 *VX_RESTRICT rhs)
{
	const float *lhs_vx = VX_REINT_CAST(const float *, &lhs->vx);
	const float *lhs_vy = VX_REINT_CAST(const float *, &lhs->vy);
	const float *lhs_vz = VX_REINT_CAST(const float *, &lhs->vz);
	const float *lhs_vw = VX_REINT_CAST(const float *, &lhs->vw);

	for (int i = 0; i < 4; ++i) {
		VX_REINT_CAST(float *, &res->vx)[i] = (lhs_vx[i] * rhs->vx.x) + (lhs_vy[i] * rhs->vx.y) + (lhs_vz[i] * rhs->vx.z) + (lhs_vw[i] * rhs->vx.w);
		VX_REINT_CAST(float *, &res->vy)[i] = (lhs_vx[i] * rhs->vy.x) + (lhs_vy[i] * rhs->vy.y) + (lhs_vz[i] * rhs->vy.z) + (lhs_vw[i] * rhs->vy.w);
		VX_REINT_CAST(float *, &res->vz)[i] = (lhs_vx[i] * rhs->vz.x) + (lhs_vy[i] * rhs->vz.y) + (lhs_vz[i] * rhs->vz.z) + (lhs_vw[i] * rhs->vz.w);
		VX_REINT_CAST(float *, &res->vw)[i] = (lhs_vx[i] * rhs->vw.x) + (lhs_vy[i] * rhs->vw.y) + (lhs_vz[i] * rhs->vw.z) + (lhs_vw[i] * rhs->vw.w);
	}
}

void mat4_look(mat4 *res, const vec3 *pos, const vec3 *front, const vec3 *up)
{
	vec3 f, s, u;
	vec3_sub(&f, front, pos);
	vec3_unit(&f, &f);

	vec3_cross(&s, &f, up);
	vec3_unit(&s, &s);

	vec3_cross(&u, &s, &f);

	res->vx.x =  s.x;
	res->vx.y =  u.x;
	res->vx.z = -f.x;
	res->vx.w = 0.0f;

	res->vy.x =  s.y;
	res->vy.y =  u.y;
	res->vy.z = -f.y;
	res->vy.w = 0.0f;

	res->vz.x =  s.z;
	res->vz.y =  u.z;
	res->vz.z = -f.z;
	res->vz.w = 0.0f;

	res->vw.x = -VX_CAST(float, vec3_dot(&s, pos));
	res->vw.y = -VX_CAST(float, vec3_dot(&u, pos));
	res->vw.z =  VX_CAST(float, vec3_dot(&f, pos));
	res->vw.w = 1.0f;
}

void mat4_rotate(mat4 *res, const mat4 *mat, float angle, const vec3 *given_axis)
{
	const float angle_cos = cosf(angle);
	const float angle_sin = sinf(angle);
	vec3 axis, other;
	vec3_unit(&axis, given_axis);
	vec3_mul(&other, &axis, 1.0f - angle_cos);

	const vec3 rx = {
		axis.x * other.x + angle_cos,
		other.x * axis.y + angle_sin * axis.z,
		other.x * axis.z - angle_sin * axis.y
	};
	const vec3 ry = {
		other.y * axis.x - angle_sin * axis.z,
		axis.y * other.y + angle_cos,
		other.y * axis.z + angle_sin * axis.x
	};
	const vec3 rz = {
		other.z * axis.x + angle_sin * axis.y,
		other.z * axis.y - angle_sin * axis.x,
		axis.z * other.z + angle_cos
	};

	const float *mat_vx = VX_REINT_CAST(const float *, &mat->vx);
	const float *mat_vy = VX_REINT_CAST(const float *, &mat->vy);
	const float *mat_vz = VX_REINT_CAST(const float *, &mat->vz);

	for (int i = 0; i < 4; ++i) {
		VX_REINT_CAST(float *, &res->vx)[i] = (mat_vx[i] * rx.x) + (mat_vy[i] * rx.y) + (mat_vz[i] * rx.z);
		VX_REINT_CAST(float *, &res->vy)[i] = (mat_vx[i] * ry.x) + (mat_vy[i] * ry.y) + (mat_vz[i] * ry.z);
		VX_REINT_CAST(float *, &res->vz)[i] = (mat_vx[i] * rz.x) + (mat_vy[i] * rz.y) + (mat_vz[i] * rz.z);
	}

	memcpy(&res->vw, &mat->vw, sizeof mat->vw);
}

void mat4_perspective(mat4 *res, float fov_y, float aspect, float near_plane, float far_plane)
{
	const float half_tan = tanf(fov_y * 0.5f);
	const float planes_dist = far_plane - near_plane;

	res->vx.x =  1.0f / (aspect * half_tan);
	res->vx.y =  0.0f;
	res->vx.z =  0.0f;
	res->vx.w =  0.0f;

	res->vy.x =  0.0f;
	res->vy.y =  1.0f / half_tan;
	res->vy.z =  0.0f;
	res->vy.w =  0.0f;

	res->vz.x =  0.0f;
	res->vz.y =  0.0f;
	res->vz.z = -(far_plane + near_plane) / planes_dist;
	res->vz.w = -1.0f;

	res->vw.x =  0.0f;
	res->vw.y =  0.0f;
	res->vw.z = -(far_plane * near_plane * 2.0f) / planes_dist;
	res->vw.w =  0.0f;
}
