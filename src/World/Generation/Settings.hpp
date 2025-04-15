#pragma once
#ifndef _SOURCE_GENERATION_SETTINGS_HDR_
#define _SOURCE_GENERATION_SETTINGS_HDR_

#include <Utility/Definitions.hpp>
#include <Rendering/FrustumCulling.hpp>

enum class WorldDirection : int8_t
{
	None = -1, // none
	Up = 0, // positive Y
	Down = 1, // negative Y
	Right = 2, // positive X
	Left = 3, // negative X
	Front = 4, // positive Z
	Back = 5, // negative Z
};

enum WorldDirectionInt
{
	IWorldDir_None = -1, // none
	IWorldDir_Up = 0, // positive Y
	IWorldDir_Down = 1, // negative Y
	IWorldDir_Right = 2, // positive X
	IWorldDir_Left = 3, // negative X
	IWorldDir_Front = 4, // positive Z
	IWorldDir_Back = 5, // negative Z
};

typedef uint8_t ObjectIDTypeof;
enum class ObjectID : ObjectIDTypeof
{
	Air,
	Grass,
	Dirt,
	Stone,
	Sand,
	Water,
	Log,
	Leaves,
	Planks,
	NumUnique,
};

enum class BiomeID {
	Plains,
	Forest,
	Desert,
	River,
	NumUnique
};

enum class ModifyWorldResult {
	Passed,
	AboveWorld,
	BelowWorld,
	NotFound,
	Invalid
};

struct WorldBlockData
{
	typedef int(*renderLookupDefinition)(int original, int target) noexcept;

	constexpr WorldBlockData(
		ObjectID, uint8_t t0, uint8_t t1, uint8_t t2, uint8_t t3, uint8_t t4, uint8_t t5,
		bool hasTransparency, 
		bool isSolid, 
		bool natureReplaceable, 
		uint8_t light,
		renderLookupDefinition renderFunctionCheck
	) noexcept :
		textures{ t0, t1, t2, t3, t4, t5 },
		hasTransparency(hasTransparency),
		isSolid(isSolid),
		natureReplaceable(natureReplaceable),
		lightEmission(light),
		renderFunction(renderFunctionCheck) {
	}

	uint8_t textures[6];
	uint8_t lightEmission;
	bool hasTransparency, isSolid, natureReplaceable;
	renderLookupDefinition renderFunction;
};

struct WorldBlockData_DEF {
	enum AtlasTextures : uint8_t
	{
		GrassTop,
		GrassSide,
		Dirt,
		Stone,
		Sand,
		Water,
		LogInner,
		LogSide,
		Leaves,
		Planks,
		Default = 0ui8,
	};

	// Easily identify which part of block data is being set
	enum BlockDataBoolean : bool {
		Transparency = true,
		Opaque = false,
		Solid = true,
		NotSolid = false,
		Replaceable = true,
		Irreplaceable = false
	};

	enum LightStages : uint8_t {
		Light0,
		Light1,
		Light2,
		Light3,
		Light4,
		Light5,
		Light6,
		LightM,
	};

	static constexpr int R_DEFAULT(int, int target) noexcept {
		return WorldBlockData_DEF::BlockIDData[target].hasTransparency;
	}
	static constexpr int R_HIDESELF(int original, int target) noexcept {
		return target != original && WorldBlockData_DEF::BlockIDData[target].hasTransparency;
	}

	static constexpr int R_NEVER(int, int) noexcept {
		return false;
	}
	static constexpr int R_ALWAYS(int, int) noexcept {
		return true;
	}

	static constexpr WorldBlockData BlockIDData[static_cast<ObjectIDTypeof>(ObjectID::NumUnique)] = {
		// ID, textures, transparency, 'solidness', replaceable, emission, render function

		{ 
			ObjectID::Air, 
			Default, Default, Default, Default, Default, Default, 
			Transparency, NotSolid, Replaceable, Light0, R_NEVER 
		},

		{ 
			ObjectID::Grass, 
			GrassTop, Dirt, GrassSide, GrassSide, GrassSide, GrassSide, 
			Opaque, Solid, Irreplaceable, Light0, R_DEFAULT
		},
		
		{
			ObjectID::Dirt,
			Dirt, Dirt, Dirt, Dirt, Dirt, Dirt,
			Opaque, Solid, Irreplaceable, Light0, R_DEFAULT
		},

		{
			ObjectID::Stone,
			Stone, Stone, Stone, Stone, Stone, Stone,
			Opaque, Solid, Irreplaceable, Light0, R_DEFAULT
		},

		{
			ObjectID::Sand,
			Sand, Sand, Sand, Sand, Sand, Sand,
			Opaque, Solid, Irreplaceable, Light0, R_DEFAULT
		},

		{
			ObjectID::Water,
			Water, Water, Water, Water, Water, Water,
			Transparency, NotSolid, Replaceable, Light0, R_HIDESELF
		},

		{
			ObjectID::Log,
			LogInner, LogInner, LogSide, LogSide, LogSide, LogSide,
			Opaque, Solid, Irreplaceable, Light0, R_DEFAULT
		},

		{
			ObjectID::Leaves,
			Leaves, Leaves, Leaves, Leaves, Leaves, Leaves,
			Opaque, Solid, Irreplaceable, Light0, R_DEFAULT
		},

		{
			ObjectID::Planks,
			Planks, Planks, Planks, Planks, Planks, Planks,
			Opaque, Solid, Irreplaceable, Light0, R_DEFAULT
		},
	};
};

struct BiomeData {
	constexpr BiomeData(float heightMult) noexcept : heightMultiplier(heightMult) {};
	float heightMultiplier;
};

struct ChunkSettings
{
	static constexpr int CHUNK_SIZE = 32;
	static constexpr int HEIGHT_COUNT = 8;
	static constexpr int RENDER_DISTANCE = 2;

	static constexpr int MAX_WORLD_HEIGHT = CHUNK_SIZE * HEIGHT_COUNT;
	static constexpr int CHUNK_SIZE_SQUARED = CHUNK_SIZE * CHUNK_SIZE;

	static constexpr PosType PCHUNK_SIZE = static_cast<PosType>(CHUNK_SIZE);
	static constexpr PosType PHEIGHT_COUNT = static_cast<PosType>(HEIGHT_COUNT);
	static constexpr PosType PRENDER_DISTANCE = static_cast<PosType>(RENDER_DISTANCE);
	static constexpr PosType PMAX_WORLD_HEIGHT = static_cast<PosType>(MAX_WORLD_HEIGHT);

	static constexpr int32_t CHUNK_SIZE_I32 = static_cast<int32_t>(CHUNK_SIZE);
	static constexpr int64_t CHUNK_SIZE_I64 = static_cast<int64_t>(CHUNK_SIZE);

	static constexpr int32_t CHUNK_BLOCKS_AMOUNT = CHUNK_SIZE_I32 * CHUNK_SIZE_I32 * CHUNK_SIZE_I32;
	static constexpr int CHUNK_BLOCKS_AMOUNT_INDEX = static_cast<int>(CHUNK_BLOCKS_AMOUNT - 1i32);
	static constexpr uint32_t UCHUNK_BLOCKS_AMOUNT = static_cast<uint32_t>(CHUNK_BLOCKS_AMOUNT);
	static constexpr uint16_t U16CHUNK_BLOCKS_AMOUNT = static_cast<uint16_t>(CHUNK_BLOCKS_AMOUNT);
	static constexpr int32_t CHUNK_UNIQUE_FACES = CHUNK_BLOCKS_AMOUNT * 6;

	static constexpr int MAX_WORLD_HEIGHT_INDEX = MAX_WORLD_HEIGHT - 1;
	static constexpr float CHUNK_NOISE_MULTIPLIER = static_cast<float>(MAX_WORLD_HEIGHT) * 0.5f;
	static constexpr double NOISE_STEP = 0.01;

	static constexpr int CHUNK_SIZE_HALF = CHUNK_SIZE / 2;
	static constexpr int CHUNK_SIZE_M1 = CHUNK_SIZE - 1;
	static constexpr float CHUNK_SIZE_FLT = static_cast<float>(CHUNK_SIZE);
	static constexpr double CHUNK_SIZE_DBL = static_cast<double>(CHUNK_SIZE);
	static constexpr int CHUNK_SIZE_POW = Math::ilogx(CHUNK_SIZE, 2);
	static constexpr int CHUNK_SIZE_POW2 = CHUNK_SIZE_POW * 2;

	static constexpr float CHUNK_SIZE_RECIP = 1.0f / CHUNK_SIZE_FLT;
	static constexpr double CHUNK_SIZE_RECIP_DBL = 1.0 / CHUNK_SIZE_DBL;

	static constexpr int FULLCHUNK_COUNT = (2 * RENDER_DISTANCE * RENDER_DISTANCE) + (2 * RENDER_DISTANCE) + 1;
	static constexpr int TOTAL_CHUNK_COUNT = FULLCHUNK_COUNT * HEIGHT_COUNT;
	static constexpr int TOTAL_CHUNK_FACES_COUNT = TOTAL_CHUNK_COUNT * 6;

	static constexpr int CHUNK_BASE_DIRT_HEIGHT = HEIGHT_COUNT / 2;
	static constexpr int CHUNK_WATER_HEIGHT = MAX_WORLD_HEIGHT / 4;
	static constexpr int CHUNK_MOUNTAIN_HEIGHT = CHUNK_WATER_HEIGHT * 3;
	static constexpr int CHUNK_LAND_MINIMUM_HEIGHT = CHUNK_WATER_HEIGHT / 2;

	static constexpr BiomeData worldBiomeData[static_cast<size_t>(BiomeID::NumUnique)] = {
		//	height
		{ 1.0f }, // Plains
		{ 1.0f }, // Forest
		{ 1.0f }, // Desert
		{ 1.0f }  // River
	};

	static WorldBlockData GetBlockData(ObjectID blockID) noexcept { 
		return WorldBlockData_DEF::BlockIDData[static_cast<ObjectIDTypeof>(blockID)];
	}

	static WorldBlockData GetBlockData(int blockID) noexcept {
		return WorldBlockData_DEF::BlockIDData[blockID];
	}

	static uint8_t GetBlockTexture(ObjectID blockID, WorldDirection direction) noexcept {
		return GetBlockData(blockID).textures[static_cast<int>(direction)];
	}

	static constexpr WorldPos worldDirections[6] = {
		{  zeroPosType,  onePosType,  zeroPosType  },	// Y+
		{  zeroPosType, -onePosType,  zeroPosType  },	// Y-
		{  onePosType,  zeroPosType,  zeroPosType  },	// X+
		{ -onePosType,  zeroPosType,  zeroPosType  },	// X-
		{  zeroPosType,  zeroPosType,  onePosType  },	// Z+
		{  zeroPosType,  zeroPosType, -onePosType  }	// Z-
	};
	static constexpr WorldPos worldDirectionsXZ[4] = {
		{  onePosType,  zeroPosType,  zeroPosType  },	// X+
		{ -onePosType,  zeroPosType,  zeroPosType  },	// X-
		{  zeroPosType,  zeroPosType,  onePosType  },	// Z+
		{  zeroPosType,  zeroPosType, -onePosType  }	// Z-
	};

	static bool InRenderDistance(WorldPos& playerOffset, const WorldPos& chunkOffset) noexcept;

	static PosType WorldPositionToOffset(PosType x) noexcept;
	static WorldPos WorldPositionToOffset(PosType x, PosType y, PosType z) noexcept;

	static int WorldToLocalPosition(PosType x) noexcept;
	static glm::ivec3 WorldToLocalPosition(PosType x, PosType y, PosType z) noexcept;

	static WorldPos GetChunkCorner(WorldPos& offset) noexcept;
	static WorldPos GetChunkCenter(WorldPos& offset) noexcept;
	static bool ChunkOnFrustum(CameraFrustum& frustum, glm::vec3& center) noexcept;

	static constexpr glm::ivec3 LocalPositionFromIndex(int index) noexcept
	{
		return {
			(index >> CHUNK_SIZE_POW) & CHUNK_SIZE_M1,
			index & CHUNK_SIZE_M1,
			(index >> CHUNK_SIZE_POW2) & CHUNK_SIZE_M1
		};
	}
	static constexpr int IndexFromLocalPosition(int x, int y, int z) noexcept
	{
		return y + (x << CHUNK_SIZE_POW) + (z << CHUNK_SIZE_POW2);
	}

	enum class ChunkBlockValueType { Full, Compressed, Air };

	typedef ObjectID(blockArray[CHUNK_BLOCKS_AMOUNT]);
	static constexpr blockArray emptyChunk{};

	struct ChunkBlockValue
	{
		virtual ObjectID GetBlockIndexed(int i) const noexcept = 0;
		virtual void SetBlockIndexed(int i, ObjectID block) noexcept = 0;

		virtual ObjectID GetBlock(int x, int y, int z) const noexcept final { return GetBlockIndexed(IndexFromLocalPosition(x, y, z)); }
		virtual void SetBlock(int x, int y, int z, ObjectID newBlock) noexcept final { SetBlockIndexed(IndexFromLocalPosition(x, y, z), newBlock); }

		virtual ChunkBlockValueType GetChunkBlockType() const noexcept = 0;
		virtual ~ChunkBlockValue() noexcept = default;
	};

	struct ChunkBlockValueFull : ChunkBlockValue
	{
		blockArray chunkBlocks{};
		ObjectID GetBlockIndexed(int i) const noexcept override { return chunkBlocks[i]; }
		void SetBlockIndexed(int i, ObjectID block) noexcept override { chunkBlocks[i] = block; }
		ChunkBlockValueType GetChunkBlockType() const noexcept override { return ChunkBlockValueType::Full; }
		~ChunkBlockValueFull() noexcept = default;
	};

	struct ChunkBlockValueCompressed : ChunkBlockValue
	{
		std::string data;
		ObjectID GetBlockIndexed(int) const noexcept override { TextFormat::log("Compressed chunk requires conversion for get"); return ObjectID::Air; }
		void SetBlockIndexed(int, ObjectID) noexcept override { TextFormat::log("Compressed chunk requires conversion for set"); }
		ChunkBlockValueType GetChunkBlockType() const noexcept override { return ChunkBlockValueType::Compressed; }
		~ChunkBlockValueCompressed() noexcept = default;
	};

	struct ChunkBlockAir : ChunkBlockValue
	{
		ObjectID GetBlockIndexed(int) const noexcept override { return ObjectID::Air; }
		ChunkBlockValueType GetChunkBlockType() const noexcept override { return ChunkBlockValueType::Air; }
		void SetBlockIndexed(int, ObjectID) noexcept override { TextFormat::log("Single chunk requires conversion for set"); }
		~ChunkBlockAir() noexcept = default;
	};

	static bool IsAirChunk(ChunkBlockValue* chunkBlocks) noexcept;
	static ChunkBlockValueFull* GetFullBlockArray(ChunkBlockValue* chunkBlocks) noexcept;
};

struct DataArrays {
	struct GenerateCoordinates
	{
		constexpr GenerateCoordinates() noexcept 
		{
			constexpr int crd = ChunkSettings::RENDER_DISTANCE;
			constexpr auto iabs = [&](int val) noexcept { return val < 0 ? -val : val; };
			for (int x = -crd, i = 0; x <= crd; ++x) {
				for (int z = -crd; z <= crd; ++z) {
					if (iabs(x) + iabs(z) <= crd) genCoords[i++] = { x, z };
				}
			}
		}

		RelativeOffset genCoords[ChunkSettings::FULLCHUNK_COUNT]{};
	};

	struct ChunkLookupTable
	{
		constexpr ChunkLookupTable() noexcept
		{
			int32_t index = 0;
			constexpr auto Overflowing = [&](int x, int y, int z) {
				constexpr int cs = ChunkSettings::CHUNK_SIZE_M1;
				return x < 0 || x > cs || y < 0 || y > cs || z < 0 || z > cs;
			};

			for (uint8_t face = 0; face < 6; ++face) {
				const glm::ivec3 epos = glm::ivec3(ChunkSettings::worldDirections[face]);
				for (int i = 0; i <= ChunkSettings::CHUNK_BLOCKS_AMOUNT_INDEX; ++i) {
					const glm::ivec3 pos = ChunkSettings::LocalPositionFromIndex(i);
					const glm::ivec3 nextPos = pos + epos;
					const int targetPosIndex = ChunkSettings::IndexFromLocalPosition(
						Math::loopAroundInteger(nextPos.x, 0, ChunkSettings::CHUNK_SIZE),
						Math::loopAroundInteger(nextPos.y, 0, ChunkSettings::CHUNK_SIZE),
						Math::loopAroundInteger(nextPos.z, 0, ChunkSettings::CHUNK_SIZE)
					);

					lookupData[index++] = {
						narrow_cast<uint16_t>(targetPosIndex),
						Overflowing(nextPos.x, nextPos.y, nextPos.z) ? face : 6ui8
					};
				}
			}
		}

		struct ChunkLookupData {
			constexpr ChunkLookupData(uint16_t i = 0ui16, uint8_t f = 0ui8) noexcept :
				blockIndex(i), nearbyIndex(f) {
			}
			uint16_t blockIndex;
			uint8_t nearbyIndex;
		};

		ChunkLookupData lookupData[ChunkSettings::CHUNK_UNIQUE_FACES]{};
	};

	static inline ChunkLookupTable lookupTable = ChunkLookupTable();
	static inline GenerateCoordinates generateCoords = GenerateCoordinates();
};

#endif // _SOURCE_GENERATION_SETTINGS_HDR_
