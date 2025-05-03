#include "Perlin.hpp"

void WorldPerlin::ChangeSeed()
{
	std::mt19937_64 gen(std::random_device{}());
	std::uniform_int_distribution<std::int64_t> dist;
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
	std::mt19937_64 gen(seed);
	std::uniform_int_distribution<int> dist(0, 255);
	for (int i = 0; i < 256; ++i) m_permutationTable[i + 256] = m_permutationTable[i] = defaultTable[dist(gen)];
}

constexpr float WorldPerlin::fade(float x) noexcept
{
	return x * x * (3.0f - 2.0f * x);
}

constexpr float WorldPerlin::grad(std::uint8_t hash, float x, float y, float z) noexcept
{
	const int h = hash & 15;
	const float u = h < 8 ? x : y;
	const float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

WorldPerlin::NoiseSpline::NoiseSpline(Spline* _splines) noexcept
{
	for (int i = 0; i < numSplines; ++i) {
		Spline &cSpline = splines[i], &nSpline = _splines[i];
		cSpline = nSpline;
	}
}

float WorldPerlin::NoiseSpline::GetNoiseHeight(float noise) const noexcept
{
	for (int i = 0; i < numSplines; ++i) {
		const Spline &currentSpline = splines[i];
		// Check if the noise value (0.0 - 1.0) is after a point to get the height value
		// between the current point and the next point at the relative noise location
		if (noise > currentSpline.normalizedLocation) {
			const Spline &nextSpline = splines[std::min(i + 1, numSplines)];
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
	return splines[numSplines - 1].heightModifier;
}
