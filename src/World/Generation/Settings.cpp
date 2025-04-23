#include "Settings.hpp"

PosType ChunkSettings::WorldPositionToOffset(PosType x) noexcept
{
	return Math::toWorld(static_cast<double>(x) * CHUNK_SIZE_RECIP_DBL);
}

WorldPos ChunkSettings::WorldPositionToOffset(PosType x, PosType y, PosType z) noexcept
{
	return { WorldPositionToOffset(x), WorldPositionToOffset(y), WorldPositionToOffset(z) };
}

int ChunkSettings::WorldToLocalPosition(PosType x) noexcept
{
	return static_cast<int>(x - (ChunkSettings::PCHUNK_SIZE * WorldPositionToOffset(x)));
}

glm::ivec3 ChunkSettings::WorldToLocalPosition(PosType x, PosType y, PosType z) noexcept
{
	return { WorldToLocalPosition(x), WorldToLocalPosition(y), WorldToLocalPosition(z) };
}

WorldPos ChunkSettings::GetChunkCorner(const WorldPos& offset) noexcept
{
	return offset * ChunkSettings::PCHUNK_SIZE;
}

WorldPos ChunkSettings::GetChunkCenter(const WorldPos& offset) noexcept
{
	constexpr PosType half = static_cast<PosType>(ChunkSettings::CHUNK_SIZE_HALF);
	const WorldPos corner = offset * ChunkSettings::PCHUNK_SIZE;
	return { corner.x + half, corner.y + half, corner.z + half };
}

bool ChunkSettings::ChunkOnFrustum(CameraFrustum& frustum, glm::vec3 center) noexcept
{
	constexpr float chunkRadius = -static_cast<float>(Math::sqrt(CHUNK_SIZE_FLT * CHUNK_SIZE_FLT));
	return frustum.top.DistToPlane(center) > chunkRadius && frustum.bottom.DistToPlane(center) > chunkRadius &&
		   frustum.right.DistToPlane(center) > chunkRadius && frustum.left.DistToPlane(center) > chunkRadius;
}

bool ChunkSettings::IsAirChunk(ChunkBlockValue* chunkBlocks) noexcept
{
	return chunkBlocks->GetChunkBlockType() == ChunkBlockValueType::Air;
}

ChunkSettings::ChunkBlockValueFull* ChunkSettings::GetFullBlockArray(ChunkBlockValue* chunkBlocks) noexcept
{
	return dynamic_cast<ChunkBlockValueFull*>(chunkBlocks);
}
