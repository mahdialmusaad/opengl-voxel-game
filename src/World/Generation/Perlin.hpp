#pragma once
#ifndef _SOURCE_GENERATION_PERLIN_HDR_
#define _SOURCE_GENERATION_PERLIN_HDR_

#include <random>

struct WorldPerlin {
	struct NoiseResult {
		NoiseResult(float cont = 0.0f, float flat = 0.0f, float temp = 0.0f, float hum = 0.0f) noexcept :
			continentalnessHeight(cont), flatness(flat), temperature(temp), humidity(hum) {
		};
		float continentalnessHeight;
		float flatness;
		float temperature;
		float humidity;
	};

	struct NoiseSpline {
		struct Spline {
			Spline() noexcept = default;
			constexpr Spline(float location, float heightmod) noexcept : normalizedLocation(location), heightModifier(heightmod) {};
			float normalizedLocation = 0.0f;
			float heightModifier = 0.0f;
		};

		NoiseSpline() noexcept = default;
		NoiseSpline(Spline *_splines) noexcept;
		float GetNoiseHeight(float noise) const noexcept;

		enum MaxSplines_ENUM : int { maxSplines = 10 };
		Spline splines[maxSplines];
	};

	NoiseSpline noiseSplines;
	std::int64_t seed = 0;

	void ChangeSeed();
	void ChangeSeed(std::int64_t newSeed);

	static constexpr double defaultY = 0.64847273f;
	static constexpr double defaultZ = 0.23569347f;
	
	float Noise1D(double x) const noexcept;
	float Noise2D(double x, double y) const noexcept;
	float Noise3D(double x, double y, double z) const noexcept;

	float Octave1D(double x, int octaves) const noexcept;
	float Octave2D(double x, double y, int octaves) const noexcept;
	float Octave3D(double x, double y, double z, int octaves) const noexcept;

private:
	template<typename T> static constexpr T lerp(T a, T b, T t) noexcept { return a + (b - a) * t; }
	static float fade(float x) noexcept;
	static float grad(std::uint8_t hash, float x, float y, float z) noexcept;

	std::uint8_t m_permutationTable[512] {};
	void ShuffleTable();
};

#endif // _SOURCE_GENERATION_PERLIN_HDR_
