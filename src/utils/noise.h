#pragma once
#ifndef SOURCE_UTILS_NOISE_VXL_HDR
#define SOURCE_UTILS_NOISE_VXL_HDR
/* Gradient noise declarations. */

#include "directives/dextern.h"

#include <stdint.h>

typedef int64_t perlin_seed;

/* Gradient noise object. */
typedef struct vxns_obj {
	uint8_t perm[512];
	perlin_seed seed;
} vxns_obj;

VX_C_START

/* Initialize the noise object with a given seed value. */
void vxns_init_seeded(vxns_obj *noise, perlin_seed seed);
/* Initialize the noise object with a random seed. */
void vxns_init(vxns_obj *noise);

double vxns_noise_1d(vxns_obj *noise, double x);
double vxns_noise_2d(vxns_obj *noise, double x, double y);
double vxns_noise_3d(vxns_obj *noise, double x, double y, double z);

double vxns_fractal_1d(vxns_obj *noise, int octaves, double x);
double vxns_fractal_2d(vxns_obj *noise, int octaves, double x, double y);
double vxns_fractal_3d(vxns_obj *noise, int octaves, double x, double y, double z);

VX_C_END

#endif
