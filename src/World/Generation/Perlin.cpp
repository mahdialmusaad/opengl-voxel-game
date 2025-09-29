#include "Perlin.hpp"

noise_object::noise_object(int64_t new_seed) : seed(new_seed)
{
	const uint8_t def_perlin_list[256] = {
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

	std::mt19937_64 gen(static_cast<std::mt19937_64::result_type>(seed));
	std::uniform_int_distribution<int> dist(0, 255);
	for (int i = 0; i < 256; ++i) m_permutations[i + 256] = m_permutations[i] = def_perlin_list[dist(gen)];
}

float noise_object::noise(double x, double y, double z) const noexcept
{
	// Perlin noise algorithm for smooth procedural terrain generation.
	// View original source from Ken Perlin at https://mrl.cs.nyu.edu/~perlin/noise/

	const vector3d flr = { ::floor(x), ::floor(y), ::floor(z) };
	const vector3i off = { static_cast<int_fast64_t>(flr.x) & 255, static_cast<int_fast64_t>(flr.y) & 255, static_cast<int_fast64_t>(flr.z) & 255 };
	
	const int  A = m_permutations[off.x] + off.y,  B = m_permutations[off.x + 1] + off.y;
	const int AA = m_permutations[A] + off.z,     AB = m_permutations[A + 1] + off.z;
	const int BA = m_permutations[B] + off.z,     BB = m_permutations[B + 1] + off.z;

	const vector3f trc = { x - flr.x, y - flr.y, z - flr.z };
	const vector3f sub = { trc.x - 1.0f, trc.y - 1.0f, trc.z - 1.0f };
	const float u = fade(trc.x), v = fade(trc.y);

	return remap01(math::lerp(
		math::lerp(
			math::lerp(grad(m_permutations[AA], trc.x, trc.y, trc.z), grad(m_permutations[BA], sub.x, trc.y, trc.z), u), 
			math::lerp(grad(m_permutations[AB], trc.x, sub.y, trc.z), grad(m_permutations[BB], sub.x, sub.y, trc.z), u),
			v
		),
		math::lerp(
			math::lerp(grad(m_permutations[AA + 1], trc.x, trc.y, sub.z), grad(m_permutations[BA + 1], sub.x, trc.y, sub.z), u), 
			math::lerp(grad(m_permutations[AB + 1], trc.x, sub.y, sub.z), grad(m_permutations[BB + 1], sub.x, sub.y, sub.z), u),
			v
		),
		fade(trc.z)
	));
}

float noise_object::octave(double x, double y, double z, int octaves) const noexcept
{
	float total = 0.0f, amplitude = 2.0f;
	double frequency = 1.0;

	for (int i = 0; i < octaves; ++i) {
		total += noise(x * frequency, y * frequency, z * frequency) * (amplitude *= 0.5f);
		frequency *= 2.0;
	}

	const float result = total / (static_cast<float>(octaves) * 0.5f);
	return result * result * result * result * (3.0f - result * 2.0f);
}

float noise_object::spline_list_obj::spline_noise_at(float noise) const noexcept
{
	for (int i = 0; i < max_splines; ++i) {
		const spline_obj &curr_spline = splines[i];
		// Check if the noise value (0.0 - 1.0) is after a point to get the height value
		// between the current point and the next point at the relative noise location
		if (noise <= curr_spline.norm_loc) continue;

		const spline_obj &next_spline = splines[math::min(i + 1, static_cast<int>(max_splines))];
		// Get the height value between the current point and the next point, 
		// specifically at where the noise value is relative to them
		// (e.g. 0.4 noise value is halfway between points 0.2 and 0.6, interpolation factor is 0.5)
		return math::lerp(
			curr_spline.noise_mult, 
			next_spline.noise_mult, 
			(noise - curr_spline.norm_loc) / (next_spline.norm_loc - curr_spline.norm_loc)
		);
	}

	// Otherwise, the noise value is above the spline points so return the last one
	return splines[max_splines - 1].noise_mult;
}


noise_obj_list::noise_obj_list(
	const int64_t (&seeds)[ne_last]
) noexcept : elevation(seeds[ne_elevation]),
	     flatness(seeds[ne_flatness]),
	     depth(seeds[ne_depth]),
	     temperature(seeds[ne_temperature]),
	     humidity(seeds[ne_humidity])
{}
