#pragma once
#ifndef _SOURCE_WORLD_CHUNK_HDR_
#define _SOURCE_WORLD_CHUNK_HDR_

#include "Generation/Settings.hpp"

struct Chunk
{
	typedef std::unordered_map<WorldPos, Chunk*, Math::WPHash> WorldMapDef;

	enum class ChunkState : std::uint8_t
	{
		Normal,
		Modified,
		UpdateRequest
	};

	struct FaceAxisData {
		std::uint32_t *instancesData;
		std::uint32_t dataIndex;
		std::uint16_t faceCount;
		std::uint16_t translucentFaceCount;
		template<typename T> T TotalFaces() const noexcept { return static_cast<T>(faceCount) + static_cast<T>(translucentFaceCount); }
	};

	struct BlockQueue {
		BlockQueue(int x, int y, int z, ObjectID id, bool natural) noexcept : pos{x, y, z}, blockID(id), natural(natural) {}
		glm::i8vec3 pos;
		ObjectID blockID;
		bool natural;
	};

	typedef std::vector<BlockQueue> BlockQueueVector;
	typedef std::unordered_map<WorldPos, BlockQueueVector, Math::WPHash> BlockQueueMap;

	ChunkSettings::BlockArray *chunkBlocks = nullptr;
	FaceAxisData chunkFaceData[6] {};

	const double positionMagnitude;
	const WorldPos *offset;

	ChunkState gameState = ChunkState::Normal;
	
	Chunk(WorldPos offset) noexcept;
	void ConstructChunk(const WorldPerlin::NoiseResult *perlinResults, BlockQueueMap &blockQueue) noexcept;
	
	void AttemptGenerateTree(BlockQueueMap &treeBlocksQueue, int x, int y, int z, const WorldPerlin::NoiseResult &noise, ObjectID log, ObjectID leaves) noexcept;
	static bool NoiseValueRand(const WorldPerlin::NoiseResult &noise, int oneInX) noexcept;

	void CalculateTerrainData(WorldMapDef &chunksMap) noexcept;

	void AllocateChunkBlocks() noexcept;

	~Chunk();
};

#endif
