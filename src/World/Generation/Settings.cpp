#include "Settings.hpp"

PosType ChunkValues::ToWorld(double a) noexcept { return static_cast<PosType>(std::floor(a)); }

bool ChunkValues::IsOnCorner(const glm::ivec3 &pos, WorldDirection dir) noexcept
{
	const int val = dir < 2 ? pos.x : dir < 4 ? pos.y : pos.z; // Determine if the direction is the X, Y or Z axis
	return val == (!(dir & 1) * ChunkValues::sizeLess); // Odd directions (left, down, back) are always in the negative directions and vice versa
}

void ChunkLookupData::CalculateLookupData(ChunkLookupData *lookupData) noexcept
{
	// Results for chunk calculation - use to check which block is next to
	// another and in which 'nearby chunk' (if it happens to be outside the current chunk)
	std::int32_t lookupIndex{};
	const std::uint8_t maxFace = static_cast<std::uint8_t>(6u);
	
	for (std::uint8_t face{}; face < maxFace; ++face) {
		const glm::ivec3 nextDirection = glm::ivec3(game.constants.worldDirections[face]);
		for (int x = 0; x < ChunkValues::size; ++x) {
			const int nextX = x + nextDirection.x;
			const bool xOverflowing = nextX < 0 || nextX >= ChunkValues::size;

			for (int y = 0; y < ChunkValues::size; ++y) {
				const int nextY = y + nextDirection.y;
				const bool yOverflowing = nextY < 0 || nextY >= ChunkValues::size;
				const bool xyOverflowing = xOverflowing || yOverflowing;

				for (int z = 0; z < ChunkValues::size; ++z) {
					const int nextZ = z + nextDirection.z;
					const bool zOverflowing = nextZ < 0 || nextZ >= ChunkValues::size;
					ChunkLookupData &data = lookupData[lookupIndex++];
					data.pos = { Math::loop(nextX, 0, ChunkValues::size), Math::loop(nextY, 0, ChunkValues::size), Math::loop(nextZ, 0, ChunkValues::size) };
					data.index = (xyOverflowing || zOverflowing) ? face : maxFace;
				}
			}
		}
	}
}
