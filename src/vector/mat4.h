#pragma once
#ifndef SOURCE_UTILS_MATRIX_VXL_HDR
#define SOURCE_UTILS_MATRIX_VXL_HDR
/* 4x4 float matrix functions definitions. */

#include "directives/dextern.h"
#include "directives/dword.h"

/* 4x4 float matrix. */
typedef struct mat4 { struct { float x, y, z, w; } vx, vy, vz, vw; } mat4;
struct vec3;

VX_C_START

/* Multiplies the matrices lhs and rhs and stores in res. */
void mat4_mul(mat4 *VX_RESTRICT res, const mat4 *VX_RESTRICT lhs, const mat4 *VX_RESTRICT rhs);
/* Creates a look matrix in res from the given vectors. */
void mat4_look(mat4 *res, const struct vec3 *position, const struct vec3 *front, const struct vec3 *up);
/* Rotates mat by angle around axis and stores in res. */
void mat4_rotate(mat4 *res, const mat4 *mat, float angle, const struct vec3 *given_axis);
/* Returns a perspective matrix in res from the given fov, window aspect ratio and plane distances. */
void mat4_perspective(mat4 *res, float fov_y, float aspect, float near_plane, float far_plane);

VX_C_END

#endif
