#include "utils/noise.h"

#include "directives/dcast.h"
#include "directives/dfast.h"

#include "utils/rand.h"

#include <stdint.h>
#include <math.h>

static uint8_t vxns_default_permutation[256] = {
	151, 160, 137, 91, 90, 15,
	131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
	190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
	88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
	77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
	102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
	135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
	5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
	223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
	129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
	251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
	49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
	138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
};

static void vxns_permutation_shuffle(vxns_obj *noise, vxrand_state *state)
{
	for (intmax_t i = 0; i < 256; ++i) {
		const uint8_t permutation = vxns_default_permutation[vxrand_get_rand(state) & 0xFFu];
		noise->perm[i] = noise->perm[i + 256] = permutation;
	}
}

void vxns_init_seeded(vxns_obj *noise, perlin_seed seed)
{
	noise->seed = seed;
	vxrand_state state;
	vxrand_init_state_seeded(&state, VX_CAST(uint64_t, seed));
	vxns_permutation_shuffle(noise, &state);
}
void vxns_init(vxns_obj *noise)
{
	vxrand_state state;
	noise->seed = VX_CAST(perlin_seed, vxrand_init_state(&state));
	vxns_permutation_shuffle(noise, &state);
}


double vxns_fractal_1d(vxns_obj *noise, int octaves, double x)
{
	double accumulated = 0.0, frequency = 1.0, amplitude = 1.0;
	for (int i = 0; i < octaves; i++) {
		accumulated += amplitude * vxns_noise_1d(noise, x * frequency);
		frequency *= 2.0; amplitude *= 0.5;
	}
	return accumulated;
}
double vxns_fractal_2d(vxns_obj *noise, int octaves, double x, double y)
{
	double accumulated = 0.0, frequency = 1.0, amplitude = 1.0;
	for (int i = 0; i < octaves; i++) {
		accumulated += amplitude * vxns_noise_2d(noise, x * frequency, y * frequency);
		frequency *= 2.0; amplitude *= 0.5;
	}
	return accumulated;
}
double vxns_fractal_3d(vxns_obj *noise, int octaves, double x, double y, double z)
{
	double accumulated = 0.0, frequency = 1.0, amplitude = 1.0;
	for (int i = 0; i < octaves; i++) {
		accumulated += amplitude * vxns_noise_3d(noise, x * frequency, y * frequency, z * frequency);
		frequency *= 2.0; amplitude *= 0.5;
	}
	return accumulated;
}


static VX_FORCEINLINE double vxns_grad_1d(intmax_t hash, double x)
{
	hash &= 0xF;
	return (1.0 + VX_CAST(double, hash & 7)) * x * (hash & 8 ? -1.0 : 1.0);
}
static VX_FORCEINLINE double vxns_grad_2d(intmax_t hash, double x, double y)
{
	hash &= 0x3F;
	const double u = hash < 4 ? x : y;
	const double v = hash < 4 ? y : x;
	return ((hash & 1) ? -u : u) + ((hash & 2) ? -2.0 * v : 2.0 * v);
}
static VX_FORCEINLINE double vxns_grad_3d(intmax_t hash, double x, double y, double z)
{
	hash &= 0xF;
	const double u = hash < 8 ? x : y;
	const double v = hash < 4 ? y : hash == 12 || hash == 14 ? x : z;
	return ((hash & 1) ? -u : u) + ((hash & 2) ? -v : v);
}

#define VX_HASH(x) (n->perm[(x) & 0xFF])

double vxns_noise_1d(vxns_obj *n, double x)
{
	const intmax_t integral = VX_CAST(int64_t, floor(x));
	const double x_trnc = x - VX_CAST(double, integral);
	const double x_trnc_m1 = x_trnc - 1.0;

	double t0 = 1.0 - x_trnc * x_trnc;
	t0 *= t0;
	double t1 = 1.0 - x_trnc_m1 * x_trnc_m1;
	t1 *= t1;

	return 0.395 * (
		(t0 * t0 * vxns_grad_1d(VX_HASH(integral), x_trnc)) +
		(t1 * t1 * vxns_grad_1d(VX_HASH(integral + 1), x_trnc_m1))
	);
}
double vxns_noise_2d(vxns_obj *n, double x, double y)
{
	const double s = (x + y) * 0.366025403;
	const intmax_t i = VX_CAST(int64_t, floor(x + s));
	const intmax_t j = VX_CAST(int64_t, floor(y + s));

	const double t = VX_CAST(double, i + j) * 0.211324865;
	const double x0 = x - (VX_CAST(double, i) - t);
	const double y0 = y - (VX_CAST(double, j) - t);

	const intmax_t i1 = x0 > y0, j1 = 1 - i1;

	const double x1 = x0 - VX_CAST(double, i1) + 0.211324865;
	const double y1 = y0 - VX_CAST(double, j1) + 0.211324865;
	const double x2 = x0 - 0.57735027;
	const double y2 = y0 - 0.57735027;

	const int gi0 = VX_HASH(i + VX_HASH(j));
	const int gi1 = VX_HASH(i + i1 + VX_HASH(j + j1));
	const int gi2 = VX_HASH(i + 1 + VX_HASH(j + 1));

	#define VX_CONTRIB_2D(v) double t##v = 0.5 - x##v * x##v - y##v * y##v, n##v;\
	if (t##v >= 0.0) { t##v *= t##v; n##v = t##v * t##v * vxns_grad_2d(gi##v, x##v, y##v); } else n##v = 0.0
	VX_CONTRIB_2D(0); VX_CONTRIB_2D(1); VX_CONTRIB_2D(2);

	return 45.23065 * (n0 + n1 + n2);
}
double vxns_noise_3d(vxns_obj *n, double x, double y, double z)
{
	#define VX_SKEW_FACTOR (1.0 / 6.0)

	const double skew = (x + y + z) * 3.333333;
	const intmax_t i = VX_CAST(int64_t, x + skew), j = VX_CAST(int64_t, y + skew), k = VX_CAST(int64_t, z + skew);
	const double t = VX_CAST(double, i + j + k) * VX_SKEW_FACTOR;
	const double x0 = x - (VX_CAST(double, i) - t), y0 = y - (VX_CAST(double, j) - t), z0 = z - (VX_CAST(double, k) - t);

	int i1, j1, k1, i2, j2, k2;
	if (x0 >= y0) {
		if (y0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
		else if (x0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1; }
		else { i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1; }
	} else {
		if (y0 < z0) { i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1; }
		else if (x0 < z0) { i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1; }
		else { i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
	}

	const double x1 = x0 - i1 + VX_SKEW_FACTOR;
	const double y1 = y0 - j1 + VX_SKEW_FACTOR;
	const double z1 = z0 - k1 + VX_SKEW_FACTOR;
	const double x2 = x0 - i2 + 2.0 * VX_SKEW_FACTOR;
	const double y2 = y0 - j2 + 2.0 * VX_SKEW_FACTOR;
	const double z2 = z0 - k2 + 2.0 * VX_SKEW_FACTOR;
	const double x3 = x0 - 1.0 + 3.0 * VX_SKEW_FACTOR;
	const double y3 = y0 - 1.0 + 3.0 * VX_SKEW_FACTOR;
	const double z3 = z0 - 1.0 + 3.0 * VX_SKEW_FACTOR;

	const int gi0 = VX_HASH(i + VX_HASH(j + VX_HASH(k)));
	const int gi1 = VX_HASH(i + i1 + VX_HASH(j + j1 + VX_HASH(k + k1)));
	const int gi2 = VX_HASH(i + i2 + VX_HASH(j + j2 + VX_HASH(k + k2)));
	const int gi3 = VX_HASH(i + 1 + VX_HASH(j + 1 + VX_HASH(k + 1)));

	#define VX_CONTRIB_3D(v) double t##v = 0.6 - x##v * x##v - y##v * y##v - z##v * z##v, n##v;\
	if (t##v >= 0.0) { t##v *= t##v; n##v = t##v * t##v * vxns_grad_3d(gi##v, x##v, y##v, z##v); } else n##v = 0.0
	VX_CONTRIB_3D(0); VX_CONTRIB_3D(1); VX_CONTRIB_3D(2); VX_CONTRIB_3D(3);

	return 32.0 * (n0 + n1 + n2 + n3);
}
