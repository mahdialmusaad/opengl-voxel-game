#pragma once
#ifndef SOURCE_GENERATION_PERLIN_VXL_HDR
#define SOURCE_GENERATION_PERLIN_VXL_HDR

#include <random>
#include "World/Generation/Vector.hpp"

namespace noise_def_vals {
	constexpr double default_y_noise = 0.64847273; // Default Y value for noise calculation
	constexpr double default_z_noise = 0.23569347; // Default Z value for noise calculation
}

struct noise_object {
	struct block_noise {
		block_noise(float cont, float flat, float temp, float hum) noexcept :
			height(cont), flatness(flat), temperature(temp), humidity(hum) {};
		float height, flatness, temperature, humidity;
	};

	struct spline_list_obj {
		struct spline_obj {
			constexpr spline_obj(float rel_location, float noise_mult_) noexcept :
			   norm_loc(rel_location), noise_mult(noise_mult_) {};
			float norm_loc = 0.0f;
			float noise_mult = 0.0f;
		};

		float spline_noise_at(float noise) const noexcept;

		enum { max_splines = 5 };
		spline_obj splines[max_splines];
	};
	
	int64_t seed;

	noise_object(int64_t new_seed);
	noise_object() noexcept = default;

	float noise(double x, double y, double z) const noexcept;
	float octave(double x, double y, double z, int octaves) const noexcept;
private:
	inline float grad(uint8_t hash, float x, float y, float z) const noexcept {
		return ((hash & 1) ? x : -x) + ((hash & 2) ? y : -y) + ((hash & 4) ? z : -z);
	}

	inline float fade(float x) const noexcept { return x * x * (3.0f - 2.0f * x); }
	inline float remap01(float result) const noexcept { return (result * 0.5f) + 0.5f; }

	uint8_t m_permutations[512];
};

// Combined noise struct
struct noise_obj_list
{
	enum noise_ind_en { ne_elevation, ne_flatness, ne_depth, ne_temperature, ne_humidity, ne_last };
	noise_obj_list(const int64_t (&seeds)[ne_last]) noexcept;
	noise_obj_list() noexcept = default;
	noise_object elevation, flatness, depth, temperature, humidity;
};

#endif // SOURCE_GENERATION_PERLIN_VXL_HDR
