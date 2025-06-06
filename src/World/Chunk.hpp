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
		
		template<typename T> 
		const T TotalFaces() const noexcept 
		{ 
			return static_cast<T>(faceCount) + static_cast<T>(translucentFaceCount);
		}
	};

	struct BlockQueue {
		BlockQueue(int x, int y, int z, ObjectID id) noexcept : x(x), y(y), z(z), blockID(id) {}
		int x, y, z;
		ObjectID blockID;
	};

	typedef std::vector<BlockQueue> BlockQueueVector;
	typedef std::unordered_map<WorldPos, BlockQueueVector, WorldPosHash> BlockQueueMap;

	ChunkSettings::ChunkBlockValue* chunkBlocks;
	FaceAxisData chunkFaceData[6] {};

	const double positionMagnitude;
	const WorldPos* offset;

	ChunkState gameState = ChunkState::Normal;
	
	Chunk(WorldPos offset) noexcept;

	BlockQueueMap ConstructChunk(WorldPerlin::NoiseResult *perlinResults) noexcept;
	void AttemptGenerateTree(BlockQueueMap& treeBlocksQueue, int x, int y, int z, const WorldPerlin::NoiseResult& noise, ObjectID log, ObjectID leaves) noexcept;

	static bool NoiseValueRand(float noiseVal, int oneInX) noexcept;

	typedef std::function<Chunk*(const WorldPos&)> ChunkGetter;
	void CalculateChunk(const ChunkGetter& findFunction) noexcept;
	void ChunkBinaryGreedMeshing(const ChunkGetter& findFunction) noexcept;

	~Chunk();
};

#endif
