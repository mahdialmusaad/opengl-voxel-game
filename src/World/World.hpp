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

	World(PlayerObject& player) noexcept;
	void DrawWorld() const noexcept;

	Chunk* WorldPositionToChunk(PosType x, PosType y, PosType z) const noexcept;
	ObjectID *EditBlock(PosType x, PosType y, PosType z) const noexcept;
	ObjectID GetBlock(PosType x, PosType y, PosType z) const noexcept;
	ModifyWorldResult SetBlock(PosType x, PosType y, PosType z, ObjectID objectID) noexcept;

	void ConvertChunkStorageType(ChunkSettings::ChunkBlockValue *blockArray) const noexcept;

	Chunk* GetChunk(WorldPos offset) const noexcept;
	bool ChunkExists(WorldPos offset) const noexcept;

	void FillBlocks(
		PosType x, PosType y, PosType z, 
		PosType tx, PosType ty, PosType tz, 
		ObjectID objectID
	) noexcept;

	void StartThreadChunkUpdate() noexcept;
	void TestChunkUpdate() noexcept;

	void UpdateWorldBuffers() noexcept;
	void SortWorldBuffers() noexcept;
private:
	void RemoveChunk(Chunk* chunk) noexcept;
	void CreateFullChunk(ChunkOffset offsetXZ) noexcept;
	void CalculateChunk(Chunk* chunk) const noexcept;
	void SetPerlinValues(WorldPerlin::NoiseResult *results, ChunkOffset offset) noexcept;

	struct TreeGenerateVars {
		constexpr TreeGenerateVars(PosType tx, PosType ty, PosType tz, const WorldPerlin::NoiseResult &result) noexcept :
			x(tx), y(ty), z(tz), noiseResult(result) {};
		const PosType x, y, z;
		const WorldPerlin::NoiseResult &noiseResult;
		const ObjectID log = ObjectID::Log;
		const ObjectID leaves = ObjectID::Leaves;
	};

	struct BlockQueue {
		constexpr BlockQueue(WorldPos pos, ObjectID id) noexcept : position(pos), blockID(id) {}
		const WorldPos position;
		const ObjectID blockID;
	};

	void GenerateTree(TreeGenerateVars vars) noexcept;
	Chunk* SetBlockSimple(PosType x, PosType y, PosType z, ObjectID objectID) const noexcept;

	uint8_t m_worldVBO;
	uint16_t m_worldVAO; 
	int32_t m_indirectCalls = 0;

	std::vector<Chunk*> m_transferChunks;
	std::unordered_map<WorldPos, std::vector<BlockQueue>, WorldPosHash> m_blockQueue;

	Chunk::ChunkGetter m_defaultGetter;
};

#endif
