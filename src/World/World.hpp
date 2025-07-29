#pragma once
#ifndef _SOURCE_WORLD_WLD_HEADER_
#define _SOURCE_WORLD_WLD_HEADER_

#include "Player/PlayerDef.hpp"

class World
{
public:
	Chunk::WorldMapDef allchunks;
	TextRenderer textRenderer;

	WorldPlayer &player;
	std::uint32_t squaresCount, renderSquaresCount, renderChunksCount;
	std::int32_t chunkRenderDistance = static_cast<std::int32_t>(4);

	World(WorldPlayer &player) noexcept;
	void DrawWorld() const noexcept;

	void DebugChunkBorders(bool drawing) noexcept;
	void DebugReset() noexcept;

	Chunk *WorldPositionToChunk(const WorldPosition &pos) const noexcept;
	ObjectID GetBlock(const WorldPosition &pos) const noexcept;
	void SetBlock(const WorldPosition &pos, ObjectID block, bool updateChunk) noexcept;

	Chunk *GetChunk(const WorldPosition &offset) const noexcept;
	PosType HighestBlockPosition(PosType x, PosType z) const noexcept;

	PosType PlayerChunkDistance(const WorldPosition &chunkOffset) const noexcept;
	bool InRenderDistance(const WorldPosition &chunkOffset) const noexcept;

	void UpdateRenderDistance(int newRenderDistance) noexcept;

	std::uintmax_t FillBlocks(WorldPosition from, WorldPosition to, ObjectID objectID) noexcept;

	void OffsetUpdate() noexcept;
	void UpdateWorldBuffers() noexcept;
	void SortWorldBuffers() noexcept;

	struct NearbyChunkData {
		Chunk *nearbyChunk;
		WorldDirection direction;
	};

	int GetNearbyChunks(const WorldPosition &offset, NearbyChunkData *nearbyData, bool includeY) const noexcept;

	int GetIndirectCalls() const noexcept;
	int GetNumChunks(bool includeHeight = true) const noexcept;

	~World() noexcept;
private:
	void SetPerlinValues(WorldPerlin::NoiseResult *results, WorldXZPosition chunkPos) noexcept;

	GLuint m_bordersVAO, m_bordersVBO, m_bordersEBO;
	GLint m_borderUniformLocation;

	GLuint m_worldVAO, m_worldInstancedVBO, m_worldPlaneVBO;
	GLsizei m_indirectCalls;
	bool canMap = false;

	typedef std::vector<Chunk::BlockQueue> BlockQueueVector;
	std::unordered_map<WorldPosition, BlockQueueVector, Math::WPHash> m_blockQueue;

	void ApplyQueue(Chunk *chunk, const BlockQueueVector &blockQueue, bool calc) noexcept;
	bool ApplyQueue(const BlockQueueVector &blockQueue, const WorldPosition &offset, bool calc) noexcept;
	bool ApplyQueue(Chunk *chunk, bool calc) noexcept;

	void AddSurrounding(Chunk::WorldMapDef &chunksMap, NearbyChunkData *nearbyData, const WorldPosition &offset, bool inclY) const noexcept;

	void ApplyUpdateRequest() noexcept;

	struct ShaderChunkFace {
		double worldPositionX;
		double worldPositionZ;
		float worldPositionY;
		std::int32_t faceIndex;
	};

	struct IndirectDrawCommand {
		GLuint count;
		GLuint instanceCount;
		GLuint first;
		GLuint baseInstance;
	};

	struct ChunkTranslucentData {
		ShaderChunkFace offsetData;
		Chunk *chunk;
	};

	Chunk::FaceAxisData **faceDataPointers = nullptr;
	ChunkTranslucentData *translucentChunks = nullptr;
	IndirectDrawCommand *worldIndirectData = nullptr;
	ShaderChunkFace *worldOffsetData = nullptr;
	WorldXZPosition *surroundingOffsets = nullptr;

	struct ChunkThreadsData { std::thread thread; Chunk::BlockQueueMap map; } *calcThreadData = new ChunkThreadsData[game.threads];
};

#endif
