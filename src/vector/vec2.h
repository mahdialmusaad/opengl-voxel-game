#pragma once
#ifndef SOURCE_UTILS_VECTOR2_VXL_HDR
#define SOURCE_UTILS_VECTOR2_VXL_HDR
/* 2D vector definitions. */

#include "directives/dextern.h"
#include "directives/dcast.h"

#include <stdint.h>
#include <math.h>

#define VX_VECTOR2_BASE(vec, type)\
typedef struct vec { type x, y; } vec;\
static inline void vec##_add(vec *res, const vec *lhs, const vec *rhs)\
{\
	res->x = lhs->x + rhs->x;\
	res->y = lhs->y + rhs->y;\
}\
static inline void vec##_sub(vec *res, const vec *lhs, const vec *rhs)\
{\
	res->x = lhs->x - rhs->x;\
	res->y = lhs->y - rhs->y;\
}\
static inline void vec##_##mul(vec *res, const vec *opr, type scalar)\
{\
	res->x = opr->x * scalar;\
	res->y = opr->y * scalar;\
}

#define VX_VECTOR2_INTEGRAL(vec, type)\
VX_VECTOR2_BASE(vec, type)\
static inline int vec##_eq(const vec *lhs, const vec *rhs)\
{\
	return lhs->x == rhs->x && lhs->y == rhs->y;\
}

#define VX_VECTOR2_FLOATING(vec, type)\
VX_VECTOR2_BASE(vec, type)\
static inline void vec##_lerp(vec *res, const vec *lhs, const vec *rhs, type interpolant)\
{\
	res->x = lhs->x + (rhs->x - lhs->x) * interpolant;\
	res->y = lhs->y + (rhs->y - lhs->y) * interpolant;\
}\
static inline type vec##_dot(const vec *lhs, const vec *rhs)\
{\
	return (lhs->x * rhs->x) + (lhs->y * rhs->y);\
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
}

VX_C_START

VX_VECTOR2_FLOATING(dvec2, double)
VX_VECTOR2_INTEGRAL(ivec2, int)

VX_C_END

#endif
