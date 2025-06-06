#pragma once
#ifndef _SOURCE_WORLD_WLD_HEADER_
#define _SOURCE_WORLD_WLD_HEADER_

#include "Player/PlayerDef.hpp"

class World
{
public:
	std::unordered_map<WorldPos, Chunk*, WorldPosHash> chunks;
	TextRenderer textRenderer;

	PlayerObject& player;
	bool threadUpdateBuffers = false;
	std::uint32_t squaresCount = 0u;
	std::int32_t chunkRenderDistance = 4;

	World(PlayerObject& player) noexcept;
	void DrawWorld() const noexcept;
	void DebugReset() noexcept;

	Chunk* WorldPositionToChunk(PosType x, PosType y, PosType z) const noexcept;
	ObjectID GetBlock(PosType x, PosType y, PosType z) const noexcept;
	void SetBlock(PosType x, PosType y, PosType z, ObjectID block, bool update = true) noexcept;

	Chunk* GetChunk(WorldPos offset) const noexcept;
	PosType HighestBlockPosition(PosType x, PosType z);

	bool InRenderDistance(WorldPos& playerOffset, const WorldPos& chunkOffset) noexcept;
	void UpdateRenderDistance(std::int32_t newRenderDistance) noexcept;

	void FillBlocks(
		PosType x, PosType y, PosType z, 
		PosType tx, PosType ty, PosType tz, 
		ObjectID objectID
	) noexcept;

	void StartThreadChunkUpdate() noexcept;
	void STChunkUpdate() noexcept;

	void UpdateWorldBuffers() noexcept;
	void SortWorldBuffers() noexcept;

	~World() noexcept;
private:
	void RemoveChunk(Chunk* chunk) noexcept;
	void CreateFullChunk(ChunkOffset offsetXZ) noexcept;
	void CalculateChunk(Chunk* chunk) const noexcept;
	void SetPerlinValues(WorldPerlin::NoiseResult *results, ChunkOffset offset) noexcept;
	
	ObjectID* EditBlock(PosType x, PosType y, PosType z, bool convert = false) const noexcept;
	ObjectID* EditBlock(Chunk* chunk, PosType x, PosType y, PosType z, bool convert = false) const noexcept;

	GLuint m_worldVAO, m_worldInstancedVBO, m_worldPlaneVBO;
	GLsizei m_indirectCalls = 0;
	bool canMap = false;

	std::vector<Chunk*> m_transferChunks;
	std::unordered_map<WorldPos, std::vector<Chunk::BlockQueue>, WorldPosHash> m_blockQueue;

	Chunk::ChunkGetter m_defaultGetter;

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
		Chunk* chunk;
	};

	Chunk::FaceAxisData** faceDataPointers;
	ChunkTranslucentData* translucentChunks;
	IndirectDrawCommand* worldIndirectData;
	ShaderChunkFace* worldOffsetData;
	WorldPerlin::NoiseResult* noiseResults = new WorldPerlin::NoiseResult[ChunkSettings::CHUNK_SIZE_SQUARED];
};

#endif
