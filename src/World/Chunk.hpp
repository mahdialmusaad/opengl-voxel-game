#pragma once
#ifndef _SOURCE_WORLD_CHUNK_HDR_
#define _SOURCE_WORLD_CHUNK_HDR_

#include "Generation/Settings.hpp"

struct Chunk
{
public:
	typedef std::unordered_map<WorldPosition, Chunk*, Math::WPHash> WorldMapDef;

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
		BlockQueue(glm::ivec3 pos, ObjectID id, bool natural) noexcept : pos(glm::i8vec3(pos)), blockID(id), natural(natural) {}
		glm::i8vec3 pos;
		ObjectID blockID;
		bool natural;
	};

	typedef std::vector<BlockQueue> BlockQueueVector;
	typedef std::unordered_map<WorldPosition, BlockQueueVector, Math::WPHash> BlockQueueMap;
	typedef BlockQueueMap::value_type BlockQueuePair;

	ChunkValues::BlockArray *chunkBlocks = nullptr;
	FaceAxisData chunkFaceData[6];

	const WorldPosition *offset;
	ChunkState gameState = ChunkState::Normal;
	
	void ConstructChunk(const WorldPerlin::NoiseResult *perlinResults, BlockQueueMap &blockQueue, const WorldPosition offset) noexcept;
	void AttemptGenerateTree(BlockQueueMap &treeBlocksQueue, int x, int y, int z, const WorldPerlin::NoiseResult &noise, ObjectID log, ObjectID leaves) noexcept;

	void AddBlockQueue(BlockQueueMap &map, const WorldPosition &offset, const BlockQueue &queue);
	
	static bool NoiseValueRand(const WorldPerlin::NoiseResult &noise, int oneInX) noexcept;
	
	void CalculateTerrainData(WorldMapDef &chunksMap) noexcept;
	void CalculateTerrainData(WorldMapDef &chunksMap, std::uint32_t *resultArray) noexcept;
	void AllocateChunkBlocks() noexcept;

	~Chunk();
};

#endif
