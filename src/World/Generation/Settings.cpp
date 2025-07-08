#include "Settings.hpp"

PosType ChunkSettings::ToWorld(double a) noexcept { return static_cast<PosType>(std::floor(a)); }

bool ChunkSettings::IsOnCorner(const glm::ivec3 &pos, WorldDirection dir) noexcept
{
	const int val = dir < 2 ? pos.x : dir < 4 ? pos.y : pos.z;
	return val == ((dir & 1) * ChunkSettings::CHUNK_SIZE_M1);
}

bool ChunkSettings::ChunkOnFrustum(const CameraFrustum &frustum, glm::vec3 center) noexcept
{
	const double chunkRadius = -ChunkSettings::CHUNK_SIZE_DBL * 6.0;
	return frustum.top.DistToPlane(center) > chunkRadius && 
		frustum.bottom.DistToPlane(center) > chunkRadius && 
		frustum.right.DistToPlane(center) > chunkRadius && 
		frustum.left.DistToPlane(center) > chunkRadius;
}

void ChunkLookupData::CalculateLookupData(ChunkLookupData (&lookupData)[ChunkSettings::CHUNK_UNIQUE_FACES]) noexcept
{
	// Results for chunk calculation - use to check which block is next to
	// another and in which 'nearby chunk' (if it happens to be outside the current chunk)
	std::int32_t lookupIndex = 0;
	const std::uint8_t maxFace = static_cast<std::uint8_t>(6u);
	
	for (std::uint8_t face = 0; face < maxFace; ++face) {
		const glm::ivec3 nextDirection = game.constants.worldDirectionsInt[face];
		for (int x = 0; x < ChunkSettings::CHUNK_SIZE; ++x) {
			const int nextX = x + nextDirection.x;
			const bool xOverflowing = nextX < 0 || nextX >= ChunkSettings::CHUNK_SIZE;

			for (int y = 0; y < ChunkSettings::CHUNK_SIZE; ++y) {
				const int nextY = y + nextDirection.y;
				const bool yOverflowing = nextY < 0 || nextY >= ChunkSettings::CHUNK_SIZE;
				const bool xyOverflowing = xOverflowing || yOverflowing;

				for (int z = 0; z < ChunkSettings::CHUNK_SIZE; ++z) {
					const int nextZ = z + nextDirection.z;
					const bool zOverflowing = nextZ < 0 || nextZ >= ChunkSettings::CHUNK_SIZE;
					ChunkLookupData &data = lookupData[lookupIndex++];

					data.pos = glm::i8vec3(
						Math::loopAround(nextX, 0, ChunkSettings::CHUNK_SIZE),
						Math::loopAround(nextY, 0, ChunkSettings::CHUNK_SIZE),
						Math::loopAround(nextZ, 0, ChunkSettings::CHUNK_SIZE)
					);
					data.index = (xyOverflowing || zOverflowing) ? face : maxFace;
				}
			}
		}
	}
}
