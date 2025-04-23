#pragma once
#ifndef _SOURCE_WORLD_CHUNK_HDR_
#define _SOURCE_WORLD_CHUNK_HDR_

#include "Generation/Settings.hpp"

struct Chunk
{
	enum class ChunkState : std::uint8_t
	{
		Normal,
		PlayerModified
	};

	struct FaceAxisData {
		std::uint32_t* instancesData;
		std::uint32_t dataIndex;
		std::uint16_t faceCount;
		std::uint16_t translucentFaceCount;
		template<typename T> inline const T TotalFaces() const noexcept 
		{ 
			return static_cast<T>(faceCount) + static_cast<T>(translucentFaceCount);
		}
	};

	ChunkSettings::ChunkBlockValue* chunkBlocks;
	FaceAxisData chunkFaceData[6] {};

	const double positionMagnitude;
	const PosType offsetX, offsetZ;
	const std::int8_t offsetY;

	ChunkState gameState = ChunkState::Normal;
	
	Chunk(WorldPos offset, WorldPerlin::NoiseResult *perlinResults) noexcept;
	WorldPos GetOffset() const noexcept;

	typedef std::function<Chunk*(const WorldPos&)> ChunkGetter;
	void CalculateChunk(const ChunkGetter& findFunction) noexcept;
	void CalculateChunkGreedy(const ChunkGetter& findFunction) noexcept;

	~Chunk();
};

#endif
