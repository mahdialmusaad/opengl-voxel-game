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

	Chunk* WorldPositionToChunk(PosType x, PosType y, PosType z) const noexcept;
	ObjectID *EditBlock(PosType x, PosType y, PosType z) const noexcept;
	ObjectID GetBlock(PosType x, PosType y, PosType z) const noexcept;
	ModifyWorldResult SetBlock(PosType x, PosType y, PosType z, ObjectID objectID) noexcept;

	void ConvertChunkStorageType(ChunkSettings::ChunkBlockValue *blockArray) const noexcept;

	Chunk* GetChunk(WorldPos offset) const noexcept;
	bool ChunkExists(WorldPos offset) const noexcept;

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

	std::uint8_t m_worldVAO; 
	std::uint8_t m_worldInstancedVBO, m_worldPlaneVBO;
	GLsizei m_indirectCalls = 0;
	bool canMap = false;

	std::vector<Chunk*> m_transferChunks;
	std::unordered_map<WorldPos, std::vector<BlockQueue>, WorldPosHash> m_blockQueue;

	Chunk::ChunkGetter m_defaultGetter;

	struct ShaderChunkOffset {
		double worldPositionX;
		double worldPositionY;
		double worldPositionZ;
		std::int32_t faceIndex;
	};

	struct IndirectDrawCommand {
		std::uint32_t count;
		std::uint32_t instanceCount;
		std::uint32_t first;
		std::uint32_t baseInstance;
	};
};

#endif
