#pragma once
#ifndef SOURCE_UTILS_RAND_VXL_HDR
#define SOURCE_UTILS_RAND_VXL_HDR
/* Pseudo-RNG (Mersenne Twister) functions declarations. */

#include "directives/dextern.h"

#include <stdint.h>

#define VX_RAND_STATE_SIZE 312

/* RNG state object. */
typedef struct vxrand_state
{
	uint64_t state[VX_RAND_STATE_SIZE];
	int64_t index;
} vxrand_state;

VX_C_START

/* Initializes the given RNG state with a seed value. */
void vxrand_init_state_seeded(vxrand_state *state, uint64_t seed);
/* Initializes the given RNG state with a random seed. */
uint64_t vxrand_init_state(vxrand_state *state);

/* Returns the next random number in the state. */
uint64_t vxrand_get_rand(vxrand_state *state);
/* Returns a random double in the range [0.0, 1.0]. */
double vxrand_get_double(vxrand_state *state);

VX_C_END

#endif
