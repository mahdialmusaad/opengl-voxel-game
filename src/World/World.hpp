#pragma once
#ifndef _SOURCE_WORLD_WLD_HEADER_
#define _SOURCE_WORLD_WLD_HEADER_

#include "Player/PlayerDef.hpp"

class World
{
public:
	Chunk::WorldMapDef allchunks;
	TextRenderer textRenderer;

	PlayerObject &player;
	std::uint32_t squaresCount = 0u;
	int chunkRenderDistance = 4;

	World(PlayerObject &player) noexcept;
	void DrawWorld() noexcept;

	void DebugChunkBorders(bool drawing) noexcept;
	void DebugReset() noexcept;

	Chunk *WorldPositionToChunk(const WorldPos &pos) const noexcept;
	ObjectID GetBlock(const WorldPos &pos) const noexcept;
	void SetBlock(const WorldPos &pos, ObjectID block, bool updateChunk) noexcept;

	Chunk *GetChunk(const WorldPos &offset) const noexcept;
	PosType HighestBlockPosition(PosType x, PosType z) const noexcept;

	bool InRenderDistance(WorldPos &playerOffset, const WorldPos &chunkOffset) noexcept;
	void UpdateRenderDistance(int newRenderDistance) noexcept;

	bool FillBlocks(WorldPos from, WorldPos to, ObjectID objectID) noexcept;

	void OffsetUpdate() noexcept;
	void UpdateWorldBuffers() noexcept;
	void SortWorldBuffers() noexcept;

	struct NearbyChunkData {
		Chunk *nearbyChunk;
		WorldDirection direction;
	};

	int GetNearbyChunks(Chunk *chunk, NearbyChunkData *nearbyData, bool includeY);
	int GetIndirectCalls() const noexcept { return static_cast<int>(m_indirectCalls); }
	int GetNumChunks(bool includeHeight = true) const noexcept { 
		const int patternChunks = (2 * chunkRenderDistance * chunkRenderDistance) + (2 * chunkRenderDistance) + 1;
		return patternChunks * (includeHeight ? ChunkSettings::HEIGHT_COUNT : 1); 
	}

	~World() noexcept;
private:
	void RemoveChunk(Chunk *chunk) noexcept;
	void SetPerlinValues(WorldPerlin::NoiseResult *results, ChunkOffset offset) noexcept;

	GLuint m_bordersVAO, m_bordersVBO, m_bordersEBO;
	GLint m_borderUniformLocation;

	GLuint m_worldVAO, m_worldInstancedVBO, m_worldPlaneVBO;
	GLsizei m_indirectCalls = 0;
	bool canMap = false;

	typedef std::vector<Chunk::BlockQueue> BlockQueueVector;
	std::unordered_map<WorldPos, BlockQueueVector, Math::WPHash> m_blockQueue;

	void ApplyQueue(Chunk *chunk, const BlockQueueVector &blockQueue, bool calc) noexcept;
	bool ApplyQueue(const BlockQueueVector &blockQueue, const WorldPos &offset, bool calc) noexcept;
	bool ApplyQueue(Chunk *chunk, bool calc) noexcept;

	void ApplyUpdateRequest() noexcept;

	struct ShaderChunkFace {
		double worldPositionX;
		double worldPositionZ;
		float worldPositionY;
		std::int32_t faceIndex;
	};

	struct IndirectDrawCommand {
		std::uint32_t count;
		std::uint32_t instanceCount;
		std::uint32_t first;
		std::uint32_t baseInstance;
	};

	struct ChunkTranslucentData {
		ShaderChunkFace offsetData;
		Chunk *chunk;
	};

	Chunk::FaceAxisData **faceDataPointers;
	ChunkTranslucentData *translucentChunks;
	IndirectDrawCommand *worldIndirectData;
	ShaderChunkFace *worldOffsetData;

	ChunkOffset *surroundingOffsets = nullptr;

	WorldPerlin::NoiseResult *noiseResults = new WorldPerlin::NoiseResult[ChunkSettings::CHUNK_SIZE_SQUARED];
};

#endif
