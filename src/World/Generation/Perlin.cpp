#include "Perlin.hpp"

void WorldPerlin::ChangeSeed()
{
	std::mt19937_64 gen(std::random_device{}());
	std::uniform_int_distribution<std::int64_t> dist(std::numeric_limits<std::int64_t>::min());
	ChangeSeed(dist(gen));
}

void WorldPerlin::ChangeSeed(std::int64_t newSeed)
{
	seed = newSeed;
	ShuffleTable();
}

float WorldPerlin::Noise1D(double x) const noexcept
{
	return Noise3D(x, defaultY, defaultZ);
}

float WorldPerlin::Noise2D(double x, double y) const noexcept
{
	return Noise3D(x, y, defaultZ);
}

float WorldPerlin::Noise3D(double x, double y, double z) const noexcept
{
	double floorX = floor(x), 
		   floorY = floor(y), 
		   floorZ = floor(z);

	int X = static_cast<int>(static_cast<std::int64_t>(floorX) & 255), 
		Y = static_cast<int>(static_cast<std::int64_t>(floorY) & 255), 
		Z = static_cast<int>(static_cast<std::int64_t>(floorZ) & 255);
	
	int A = m_permutationTable[X] + Y, B = m_permutationTable[X + 1] + Y;
	int AA = m_permutationTable[A] + Z, AB = m_permutationTable[A + 1] + Z;
	int BA = m_permutationTable[B] + Z, BB = m_permutationTable[B + 1] + Z;

	float fx = static_cast<float>(x - floorX), nX = fx - 1.0f, 
		  fy = static_cast<float>(y - floorY), nY = fy - 1.0f, 
		  fz = static_cast<float>(z - floorZ), nZ = fz - 1.0f;

	float u = fade(fx), v = fade(fy);

	return lerp(
		lerp(
			lerp(grad(m_permutationTable[AA], fx, fy, fz), grad(m_permutationTable[BA], nX, fy, fz), u), 
			lerp(grad(m_permutationTable[AB], fx, nY, fz), grad(m_permutationTable[BB], nX, nY, fz), u), v
		), 
		lerp(
			lerp(grad(m_permutationTable[AA + 1], fx, fy, nZ), grad(m_permutationTable[BA + 1], nX, fy, nZ), u), 
			lerp(grad(m_permutationTable[AB + 1], fx, nY, nZ), grad(m_permutationTable[BB + 1], nX, nY, nZ), u), v
		), 
		fade(fz)
	);
}

float WorldPerlin::Octave1D(double x, int octaves) const noexcept
{
	float total = 0.0f, frequency = 1.0f, amplitude = 1.0f, tvalue = 0.0f;

	for (int i = 0; i < octaves; ++i) {
		tvalue += Noise1D(x * frequency) * amplitude;
		total += amplitude;
		amplitude *= 0.5f;
		frequency *= 2.0f;
	}

	return tvalue / total;
}

float WorldPerlin::Octave2D(double x, double y, int octaves) const noexcept
{
	float total = 0.0f, frequency = 1.0f, amplitude = 1.0f, tvalue = 0.0f;

	for (int i = 0; i < octaves; ++i) {
		tvalue += Noise2D(x * frequency, y * frequency) * amplitude;
		total += amplitude;
		amplitude *= 0.5f;
		frequency *= 2.0f;
	}

	return tvalue / total;
}

float WorldPerlin::Octave3D(double x, double y, double z, int octaves) const noexcept
{
	float total = 0.0f, frequency = 1.0f, amplitude = 1.0f, tvalue = 0.0f;

	for (int i = 0; i < octaves; ++i) {
		tvalue += Noise3D(x * frequency, y * frequency, z * frequency) * amplitude;
		total += amplitude;
		amplitude *= 0.5f;
		frequency *= 2.0f;
	}

	return tvalue / total;
}

void WorldPerlin::ShuffleTable()
{
	static const std::uint8_t defaultPerlinTable[256] = {
		151u, 160u, 137u, 91u, 90u, 15u,
		131u, 13u, 201u, 95u, 96u, 53u, 194u, 233u, 7u, 225u, 140u, 36u, 103u, 30u, 69u, 142u, 8u, 99u, 37u, 240u, 21u, 10u, 23u,
		190u, 6u, 148u, 247u, 120u, 234u, 75u, 0u, 26u, 197u, 62u, 94u, 252u, 219u, 203u, 117u, 35u, 11u, 32u, 57u, 177u, 33u,
		88u, 237u, 149u, 56u, 87u, 174u, 20u, 125u, 136u, 171u, 168u, 68u, 175u, 74u, 165u, 71u, 134u, 139u, 48u, 27u, 166u,
		77u, 146u, 158u, 231u, 83u, 111u, 229u, 122u, 60u, 211u, 133u, 230u, 220u, 105u, 92u, 41u, 55u, 46u, 245u, 40u, 244u,
		102u, 143u, 54u, 65u, 25u, 63u, 161u, 1u, 216u, 80u, 73u, 209u, 76u, 132u, 187u, 208u, 89u, 18u, 169u, 200u, 196u,
		135u, 130u, 116u, 188u, 159u, 86u, 164u, 100u, 109u, 198u, 173u, 186u, 3u, 64u, 52u, 217u, 226u, 250u, 124u, 123u,
		5u, 202u, 38u, 147u, 118u, 126u, 255u, 82u, 85u, 212u, 207u, 206u, 59u, 227u, 47u, 16u, 58u, 17u, 182u, 189u, 28u, 42u,
		223u, 183u, 170u, 213u, 119u, 248u, 152u, 2u, 44u, 154u, 163u, 70u, 221u, 153u, 101u, 155u, 167u, 43u, 172u, 9u,
		129u, 22u, 39u, 253u, 19u, 98u, 108u, 110u, 79u, 113u, 224u, 232u, 178u, 185u, 112u, 104u, 218u, 246u, 97u, 228u,
		251u, 34u, 242u, 193u, 238u, 210u, 144u, 12u, 191u, 179u, 162u, 241u, 81u, 51u, 145u, 235u, 249u, 14u, 239u, 107u,
		49u, 192u, 214u, 31u, 181u, 199u, 106u, 157u, 184u, 84u, 204u, 176u, 115u, 121u, 50u, 45u, 127u, 4u, 150u, 254u,
		138u, 236u, 205u, 93u, 222u, 114u, 67u, 29u, 24u, 72u, 243u, 141u, 128u, 195u, 78u, 66u, 215u, 61u, 156u, 180u
	};

	std::mt19937_64 gen(seed);
	std::uniform_int_distribution<int> dist(0, 255);
	for (int i = 0; i < 256; ++i) m_permutationTable[i + 256] = m_permutationTable[i] = defaultPerlinTable[dist(gen)];
}

float WorldPerlin::fade(float x) noexcept
{
	return x * x * (3.0f - 2.0f * x);
}

float WorldPerlin::grad(std::uint8_t hash, float x, float y, float z) noexcept
{
	const int h = hash & 15;
	const float u = h < 8 ? x : y;
	const float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

WorldPerlin::NoiseSpline::NoiseSpline(Spline *_splines) noexcept
{
	for (int i = 0; i < maxSplines; ++i) {
		Spline &cSpline = splines[i], &nSpline = _splines[i];
		cSpline = nSpline;
	}
}

float WorldPerlin::NoiseSpline::GetNoiseHeight(float noise) const noexcept
{
	for (int i = 0; i < maxSplines; ++i) {
		const Spline &currentSpline = splines[i];
		// Check if the noise value (0.0 - 1.0) is after a point to get the height value
		// between the current point and the next point at the relative noise location
		if (noise > currentSpline.normalizedLocation) {
			const Spline &nextSpline = splines[std::min(i + 1, static_cast<int>(maxSplines))];
			// Get the height value between the current point and the next point, 
			// specifically at where the noise value is relative to them
			// (e.g. 0.4 noise value is halfway between points 0.2 and 0.6, interpolation factor is 0.5)
			return WorldPerlin::lerp(
				currentSpline.heightModifier, 
				nextSpline.heightModifier, 
				(noise - currentSpline.normalizedLocation) / (nextSpline.normalizedLocation - currentSpline.normalizedLocation)
			);
		}
	}

	// Otherwise, the noise value is above the spline points so return the last one
	return splines[maxSplines - 1].heightModifier;
}
