#pragma once
#ifndef _SOURCE_GENERATION_PERLIN_HDR_
#define _SOURCE_GENERATION_PERLIN_HDR_

#include <random>

// Editable settings to do with terrain generation and noise calculations.
namespace NoiseValues {
	constexpr double defaultY = 0.64847273; // Default Y value for noise calculation
	constexpr double defaultZ = 0.23569347; // Default Z value for noise calculation

	constexpr double noiseStep = 0.01; // How much to traverse in the 'noise map' per block.

	constexpr float maxTerrain = 200.0f; // Maximum height for terrain (structures can still be higher).
	constexpr float minSurface = 50.0f; // Minimum height for surface (including underwater surface).

	constexpr float terrainRange = maxTerrain - minSurface; // Do not change

	struct LookupVec2 { float x, y; } constexpr noiseLookupVec2[4] = {
		{ 1.0f, 1.0f }, { -1.0f, 1.0f }, { -1.0f, -1.0f }, { 1.0f, -1.0f }
	};
}

struct WorldPerlin {
	struct NoiseResult {
		NoiseResult() noexcept = default;
		NoiseResult(double x, double z, float cont, float flat, float temp, float hum) noexcept :
			posX(x), posZ(z), landHeight(cont), flatness(flat), temperature(temp), humidity(hum) {};
		double posX, posZ;
		float landHeight, flatness, temperature, humidity;
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

		enum { maxSplines = 10 };
		Spline splines[maxSplines];
	};

	NoiseSpline noiseSplines;
	std::int64_t seed = 0;

	void ChangeSeed();
	void ChangeSeed(std::int64_t newSeed);

	float GetNoise(double x, double y, double z) const noexcept;
	float GetOctave(double x, double y, double z, int octaves) const noexcept;
private:
	static float lerp(float a, float b, float t) noexcept { return a + (b - a) * t; };
	float getDot(const NoiseValues::LookupVec2 &vec, int permVal) const noexcept {
		const NoiseValues::LookupVec2 &lv2 = NoiseValues::noiseLookupVec2[permVal & 3];
		return vec.x * lv2.x + vec.y * lv2.y;
	}
	float grad(std::uint8_t hash, float x, float y, float z) const noexcept;

	float fade(float x) const noexcept { return x * x * (3.0f - 2.0f * x); }
	float RemapNoise(float result) const noexcept { return (result * 0.5f) + 0.5f; }

	std::uint8_t m_permutationTable[512] {};
};

#endif // _SOURCE_GENERATION_PERLIN_HDR_
