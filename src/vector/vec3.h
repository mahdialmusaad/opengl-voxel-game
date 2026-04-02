#pragma once
#ifndef SOURCE_UTILS_VECTOR3_VXL_HDR
#define SOURCE_UTILS_VECTOR3_VXL_HDR
/* 3D vector definitions. */

#include "directives/dextern.h"
#include "directives/dcast.h"

#include <math.h>

#define VX_VECTOR3_BASE(vec, type)\
typedef struct vec { type x, y, z; } vec;\
static inline void vec##_add(vec *res, const vec *lhs, const vec *rhs)\
{\
	res->x = lhs->x + rhs->x;\
	res->y = lhs->y + rhs->y;\
	res->z = lhs->z + rhs->z;\
}\
static inline void vec##_sub(vec *res, const vec *lhs, const vec *rhs)\
{\
	res->x = lhs->x - rhs->x;\
	res->y = lhs->y - rhs->y;\
	res->z = lhs->z - rhs->z;\
}\
static inline void vec##_##mul(vec *res, const vec *opr, type scalar)\
{\
	res->x = opr->x * scalar;\
	res->y = opr->y * scalar;\
	res->z = opr->z * scalar;\
}

#define VX_VECTOR3_INTEGRAL(vec, type)\
VX_VECTOR3_BASE(vec, type)\
static inline int vec##_eq(const vec *lhs, const vec *rhs)\
{\
	return lhs->x == rhs->x && lhs->y == rhs->y && lhs->z == rhs->z;\
}

#define VX_VECTOR3_FLOATING(vec, type)\
VX_VECTOR3_BASE(vec, type)\
static inline void vec##_cross(vec *res, const vec *lhs, const vec *rhs)\
{\
	res->x = lhs->y * rhs->z - lhs->z * rhs->y;\
	res->y = lhs->z * rhs->x - lhs->x * rhs->z;\
	res->z = lhs->x * rhs->y - lhs->y * rhs->x;\
}\
static inline void vec##_lerp(vec *res, const vec *lhs, const vec *rhs, type interpolant)\
{\
	res->x = lhs->x + (rhs->x - lhs->x) * interpolant;\
	res->y = lhs->y + (rhs->y - lhs->y) * interpolant;\
	res->z = lhs->z + (rhs->z - lhs->z) * interpolant;\
}\
static inline type vec##_dot(const vec *lhs, const vec *rhs)\
{\
	return (lhs->x * rhs->x) + (lhs->y * rhs->y) + (lhs->z * rhs->z);\
}\
static inline double vec##_length(const vec *opr)\
{\
	return sqrt(VX_CAST(double, vec##_dot(opr, opr)));\
}\
static inline void vec##_unit(vec *res, const vec *opr)\
{\
	const type inv_len = VX_CAST(type, 1.0 / vec##_length(opr));\
	res->x = opr->x * inv_len;\
	res->y = opr->y * inv_len;\
	res->z = opr->z * inv_len;\
}

VX_C_START

VX_VECTOR3_FLOATING(dvec3, double)
VX_VECTOR3_FLOATING(vec3, float)
VX_VECTOR3_INTEGRAL(ivec3, int)

VX_C_END

#endif
