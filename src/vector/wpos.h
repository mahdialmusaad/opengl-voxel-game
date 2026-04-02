#pragma once
#ifndef SOURCE_UTILS_VECTORPOS_VXL_HDR
#define SOURCE_UTILS_VECTORPOS_VXL_HDR

#include "directives/dextern.h"

#include <inttypes.h>
#if !defined (INTMAX_C)
#include <stdint.h>
#endif

/* Type used to store a position value. */
typedef intmax_t pos_type;
/* String format for positioning type. */
#define VXFMTPOS "%" PRIiMAX

typedef struct wpos { pos_type x, y, z; } wpos;

VX_C_START

static inline void wpos_add(wpos *res, const wpos *lhs, const wpos *rhs)
{
	res->x = lhs->x + rhs->x;
	res->y = lhs->y + rhs->y;
	res->z = lhs->z + rhs->z;
}
static inline void wpos_sub(wpos *res, const wpos *lhs, const wpos *rhs)
{
	res->x = lhs->x - rhs->x;
	res->y = lhs->y - rhs->y;
	res->z = lhs->z - rhs->z;
}
static inline int wpos_eq(const wpos *lhs, const wpos *rhs)
{
	return lhs->x == rhs->x && lhs->y == rhs->y && lhs->z == rhs->z;
}
static inline int wpos_neq(const wpos *lhs, const wpos *rhs)
{
	return lhs->x != rhs->x || lhs->y != rhs->y || lhs->z != rhs->z;
}

VX_C_END

#endif
