#include "utils/rand.h"

#include "directives/dcast.h"
#include "directives/dos.h"

#include "graphics/glfw.h"

/* Use OS-specific RNG. */
#if VX_WINDOWS == 1
# define _CRT_RAND_S
# include <stdlib.h>
#else
# include <sys/random.h>
#endif

#include <stdint.h>

/* Generate initial seed from OS-provided random function (or time fallback). */
static uint64_t vxrand_get_seed(VX_NO_ARG)
{
#if VX_WINDOWS == 1
	unsigned int rand_res[2];
	if (rand_s(rand_res) != 0) goto fallback_rnd;
	if (rand_s(rand_res + 1) != 0) goto fallback_rnd;
	return *VX_REINT_CAST(uint64_t *, rand_res);
#elif VX_UNIX == 1
	uint64_t rnd;
	if (getrandom(&rnd, sizeof rnd, 0) != sizeof rnd) goto fallback_rnd;
	return rnd;
#endif
fallback_rnd:
	{
		const uint64_t elapsed_micros = VX_CAST(uint64_t, glfwGetTime() * 1000.0 * 1000.0);
		return VX_CAST(uint64_t, elapsed_micros) ^ ((UINT64_C(0x12BD6AA) + (elapsed_micros >> 3u)) * UINT64_C(0xD2EFF5D5)) ^ UINT64_C(0x3201C3C8);
	}
}

/* Initializes the given RNG state with a seed value. */
void vxrand_init_state_seeded(vxrand_state *state, uint64_t seed)
{
	*state->state = seed;
	state->index = 0;
	for (int i = 1; i < VX_RAND_STATE_SIZE; i++) state->state[i] = (seed = UINT64_C(0x5851F42D4C957F2D) * (seed ^ (seed >> 62u)) + VX_CAST(uint64_t, i));
}
/* Initializes the given RNG state with a random seed. */
uint64_t vxrand_init_state(vxrand_state *state)
{
	uint64_t seed = vxrand_get_seed();
	vxrand_init_state_seeded(state, seed);
	return seed;
}

uint64_t vxrand_get_rand(vxrand_state *state)
{
	int64_t ind = state->index;

	int64_t j = ind - (VX_RAND_STATE_SIZE - 1);
	if (j < 0) j += VX_RAND_STATE_SIZE;

	uint64_t x = (state->state[ind] & (UINT64_C(0xFFFFFFFF) << 31u)) | (state->state[j] & (UINT64_C(0xFFFFFFFF) >> 2u));

	j = ind - (VX_RAND_STATE_SIZE - 156);
	if (j < 0) j += VX_RAND_STATE_SIZE;

	state->state[ind++] = x = state->state[j] ^ ((x >> 1u) ^ ((UINT64_C(0xB5026F5AA96619E9)) * (x & 1u)));
	state->index = ind * (ind < VX_RAND_STATE_SIZE);
	
	uint64_t y = x ^ (x >> 29u);
	y ^= (y << 17u) & UINT64_C(0x71D67FFFEDA60000);
	y ^= (y << 37u) & UINT64_C(0xFFF7EEE000000000);
	return y ^ (y >> 43u);
}

double vxrand_get_double(vxrand_state *state)
{
	return VX_CAST(double, vxrand_get_rand(state)) * (1.0 / VX_CAST(double, UINT64_MAX));
}
