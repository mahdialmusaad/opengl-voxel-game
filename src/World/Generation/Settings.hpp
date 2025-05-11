#pragma once
#ifndef _SOURCE_GENERATION_SETTINGS_HDR_
#define _SOURCE_GENERATION_SETTINGS_HDR_

#include "Globals/Definitions.hpp"
#include "Rendering/Frustum.hpp"

enum class WorldDirection : std::int8_t
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

// Upcoming, for reference now
enum class BiomeID 
{
	Plains,
	Forest,
	Desert,
	River,
	NumUnique
};

typedef std::uint8_t ObjectIDTypeof; // Change when there are enough unique IDs
typedef std::uint8_t TextureIDTypeof; // Also change this when there are enough textures

// List of all different blocks and (possibly) items
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

// Block properties
struct WorldBlockData
{
	typedef int(*renderLookupDefinition)(const WorldBlockData& original, const WorldBlockData& target);

	constexpr WorldBlockData(
		ObjectID oid, const char* name,
		TextureIDTypeof t0, TextureIDTypeof t1, TextureIDTypeof t2, TextureIDTypeof t3, TextureIDTypeof t4, TextureIDTypeof t5,
		bool hasTransparency, 
		bool isSolid, 
		bool natureReplaceable, 
		std::uint8_t light,
		renderLookupDefinition renderFunc
	) noexcept :
		id(static_cast<int>(oid)),
		name(name),
		textures{ t0, t1, t2, t3, t4, t5 },
		lightEmission(light),
		hasTransparency(hasTransparency),
		isSolid(isSolid),
		natureReplaceable(natureReplaceable),
		notObscuredBy(*renderFunc) {
	}

	const int id;
	const char* name;
	const std::uint8_t textures[6];
	const std::uint8_t lightEmission;
	const bool hasTransparency, isSolid, natureReplaceable;
	const renderLookupDefinition notObscuredBy;
};

// Property definitions for all object IDs as seen above
namespace WorldBlockData_DEF 
{
	enum AtlasTextures : TextureIDTypeof
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
		DefaultTex = 0u,
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

	enum LightStages : std::uint8_t {
		LightN,
		Light1,
		Light2,
		Light3,
		Light4,
		Light5,
		Light6,
		LightX,
	};

	// Functions each block uses when determining visibility next to another block
	typedef const WorldBlockData& WBD;
	constexpr int R_Never(WBD, WBD) { return false; }
	constexpr int R_Always(WBD, WBD) { return true; }
	constexpr int R_HideSelf(WBD original, WBD target) { return original.id != target.id && target.hasTransparency; }
	constexpr int R_Default(WBD, WBD target) { return target.hasTransparency; }

	// All blocks and their properties
	constexpr WorldBlockData BlockIDData[static_cast<int>(ObjectID::NumUnique)] = {
	// ID, textures, transparency, 'solidness', replaceable, emission, render function index
		{ 
			ObjectID::Air, "Air",
			DefaultTex, DefaultTex, DefaultTex, DefaultTex, DefaultTex, DefaultTex, 
			Transparency, NotSolid, Replaceable, LightN, R_Never
		},
		{ 
			ObjectID::Grass, "Grass",
			GrassTop, Dirt, GrassSide, GrassSide, GrassSide, GrassSide, 
			Opaque, Solid, Irreplaceable, LightN, R_Default
		},
		{
			ObjectID::Dirt, "Dirt",
			Dirt, Dirt, Dirt, Dirt, Dirt, Dirt,
			Opaque, Solid, Irreplaceable, LightN, R_Default
		},
		{
			ObjectID::Stone, "Stone",
			Stone, Stone, Stone, Stone, Stone, Stone,
			Opaque, Solid, Irreplaceable, LightN, R_Default
		},
		{
			ObjectID::Sand, "Sand",
			Sand, Sand, Sand, Sand, Sand, Sand,
			Opaque, Solid, Irreplaceable, LightN, R_Default
		},
		{
			ObjectID::Water, "Water",
			Water, Water, Water, Water, Water, Water,
			Transparency, NotSolid, Replaceable, LightN, R_HideSelf
		},
		{
			ObjectID::Log, "Log",
			LogInner, LogInner, LogSide, LogSide, LogSide, LogSide,
			Opaque, Solid, Irreplaceable, LightN, R_Default
		},
		{
			ObjectID::Leaves, "Leaves",
			Leaves, Leaves, Leaves, Leaves, Leaves, Leaves,
			Opaque, Solid, Replaceable, LightN, R_Default
		},
		{
			ObjectID::Planks, "Planks",
			Planks, Planks, Planks, Planks, Planks, Planks,
			Opaque, Solid, Irreplaceable, LightN, R_Default
		},
	};
};


// Game settings
namespace ChunkSettings
{
	constexpr int CHUNK_SIZE = 32; // (POWER OF 2 ONLY) Changing this requires the world shader to be updated as well.
	constexpr int HEIGHT_COUNT = 8; // How many chunks in a 'full chunk'

	// Chunk generation settings (feel free to edit)!
	constexpr int CHUNK_BASE_DIRT_HEIGHT = HEIGHT_COUNT / 2;
	constexpr int CHUNK_WATER_HEIGHT = CHUNK_SIZE * HEIGHT_COUNT / 4;
	constexpr int CHUNK_MOUNTAIN_HEIGHT = CHUNK_WATER_HEIGHT * 3;
	constexpr int CHUNK_LAND_MINIMUM_HEIGHT = CHUNK_WATER_HEIGHT / 2;
	constexpr double NOISE_STEP = 0.01;
	
	// Calculation shortcuts - do not change!
	constexpr int CHUNK_SIZE_SQUARED = CHUNK_SIZE * CHUNK_SIZE;
	constexpr int MAX_WORLD_HEIGHT = CHUNK_SIZE * HEIGHT_COUNT;

	constexpr PosType PCHUNK_SIZE = static_cast<PosType>(CHUNK_SIZE);
	constexpr PosType PHEIGHT_COUNT = static_cast<PosType>(HEIGHT_COUNT);
	constexpr PosType PMAX_WORLD_HEIGHT = static_cast<PosType>(MAX_WORLD_HEIGHT);

	constexpr std::int32_t CHUNK_SIZE_I32 = static_cast<std::int32_t>(CHUNK_SIZE);
	constexpr std::int64_t CHUNK_SIZE_I64 = static_cast<std::int64_t>(CHUNK_SIZE);

	constexpr std::int32_t CHUNK_BLOCKS_AMOUNT = CHUNK_SIZE_I32 * CHUNK_SIZE_I32 * CHUNK_SIZE_I32;
	constexpr int CHUNK_BLOCKS_AMOUNT_INDEX = static_cast<int>(CHUNK_BLOCKS_AMOUNT - 1);
	constexpr std::uint32_t UCHUNK_BLOCKS_AMOUNT = static_cast<std::uint32_t>(CHUNK_BLOCKS_AMOUNT);
	constexpr std::uint16_t U16CHUNK_BLOCKS_AMOUNT = static_cast<std::uint16_t>(CHUNK_BLOCKS_AMOUNT);
	constexpr std::int32_t CHUNK_UNIQUE_FACES = CHUNK_BLOCKS_AMOUNT * 6;

	constexpr int MAX_WORLD_HEIGHT_INDEX = MAX_WORLD_HEIGHT - 1;
	constexpr float CHUNK_NOISE_MULTIPLIER = static_cast<float>(MAX_WORLD_HEIGHT) * 0.5f;

	constexpr int CHUNK_SIZE_HALF = CHUNK_SIZE / 2;
	constexpr int CHUNK_SIZE_M1 = CHUNK_SIZE - 1;
	constexpr float CHUNK_SIZE_FLT = static_cast<float>(CHUNK_SIZE);
	constexpr double CHUNK_SIZE_DBL = static_cast<double>(CHUNK_SIZE);
	constexpr int CHUNK_SIZE_POW = Math::ilogx(CHUNK_SIZE, 2);
	constexpr int CHUNK_SIZE_POW2 = CHUNK_SIZE_POW * 2;

	constexpr float CHUNK_SIZE_RECIP = 1.0f / CHUNK_SIZE_FLT;
	constexpr double CHUNK_SIZE_RECIP_DBL = 1.0 / CHUNK_SIZE_DBL;

	// Upcoming biomes settings
	// struct BiomeData {
	//     constexpr BiomeData(float heightMult) noexcept : heightMultiplier(heightMult) {};
	//     float heightMultiplier;
	// } worldBiomeData[static_cast<size_t>(BiomeID::NumUnique)] = {
	// //	 height
	// 	{ 1.0f }, // Plains
	// 	{ 1.0f }, // Forest
	// 	{ 1.0f }, // Desert
	// 	{ 1.0f }  // River
	// };

	constexpr const WorldBlockData& GetBlockData(ObjectID blockID) noexcept { 
		return WorldBlockData_DEF::BlockIDData[static_cast<ObjectIDTypeof>(blockID)];
	}

	constexpr const WorldBlockData& GetBlockData(int blockID) noexcept {
		return WorldBlockData_DEF::BlockIDData[blockID];
	}

	constexpr TextureIDTypeof GetBlockTexture(ObjectID blockID, int direction) noexcept {
		return GetBlockData(blockID).textures[direction];
	}
	constexpr TextureIDTypeof GetBlockTexture(ObjectID blockID, WorldDirection direction) noexcept {
		return GetBlockData(blockID).textures[static_cast<int>(direction)];
	}

	constexpr WorldPos worldDirections[6] = {
		{  0,  1,  0  },	// Y+
		{  0, -1,  0  },	// Y-
		{  1,  0,  0  },	// X+
		{ -1,  0,  0  },	// X-
		{  0,  0,  1  },	// Z+
		{  0,  0, -1  }	// Z-
	};

	constexpr WorldPos worldDirectionsXZ[4] = {
		{  1,  0,  0  },	// X+
		{ -1,  0,  0  },	// X-
		{  0,  0,  1  },	// Z+
		{  0,  0, -1  }	// Z-
	};

	PosType WorldPositionToOffset(PosType x) noexcept;
	WorldPos WorldPositionToOffset(PosType x, PosType y, PosType z) noexcept;

	int WorldToLocalPosition(PosType x) noexcept;
	glm::ivec3 WorldToLocalPosition(PosType x, PosType y, PosType z) noexcept;

	WorldPos GetChunkCorner(const WorldPos& offset) noexcept;
	WorldPos GetChunkCenter(const WorldPos& offset) noexcept;
	bool ChunkOnFrustum(CameraFrustum& frustum, glm::vec3 center) noexcept;

	constexpr glm::ivec3 LocalPositionFromIndex(int index) noexcept
	{
		return {
			(index >> CHUNK_SIZE_POW) & CHUNK_SIZE_M1, 	// X
			index & CHUNK_SIZE_M1,						// Y
			(index >> CHUNK_SIZE_POW2) & CHUNK_SIZE_M1	// Z
		};
	}
	constexpr int IndexFromLocalPosition(int x, int y, int z) noexcept
	{
		return y + (x << CHUNK_SIZE_POW) + (z << CHUNK_SIZE_POW2); // Y: bits 0-5, X: bits 5-10, Z: bits 10-15
	}

	enum class ChunkBlockValueType { Full, Compressed, Air };

	typedef ObjectID(blockArray[CHUNK_BLOCKS_AMOUNT]);
	constexpr blockArray emptyChunk{};

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

	bool IsAirChunk(ChunkBlockValue* chunkBlocks) noexcept;
	ChunkBlockValueFull* GetFullBlockArray(ChunkBlockValue* chunkBlocks) noexcept;
};

struct ChunkLookupTable
{
	constexpr ChunkLookupTable() noexcept
	{
		constexpr int cs = ChunkSettings::CHUNK_SIZE_M1;
		std::size_t index = 0u;

		for (std::uint8_t face = 0; face < 6; ++face) {
			const glm::ivec3 epos = glm::ivec3(ChunkSettings::worldDirections[face]);
			for (int i = 0; i <= ChunkSettings::CHUNK_BLOCKS_AMOUNT_INDEX; ++i) {
				const glm::ivec3 crnt = ChunkSettings::LocalPositionFromIndex(i);
				const glm::ivec3 nxt = crnt + epos;
				const int targetPosIndex = ChunkSettings::IndexFromLocalPosition(
					Math::loopAroundInteger(nxt.x, 0, ChunkSettings::CHUNK_SIZE),
					Math::loopAroundInteger(nxt.y, 0, ChunkSettings::CHUNK_SIZE),
					Math::loopAroundInteger(nxt.z, 0, ChunkSettings::CHUNK_SIZE)
				);

				ChunkLookupData& data = lookupData[index++];
				data.blockIndex = static_cast<std::uint16_t>(targetPosIndex);
				const bool isOverflowing = nxt.x < 0 || nxt.x > cs || nxt.y < 0 || nxt.y > cs || nxt.z < 0 || nxt.z > cs;
				data.nearbyIndex = isOverflowing ? face : static_cast<std::uint8_t>(6u);
			}
		}
	}

	struct ChunkLookupData {
		std::uint16_t blockIndex = 0u;
		std::uint8_t nearbyIndex = 0u;
	};

	ChunkLookupData lookupData[ChunkSettings::CHUNK_UNIQUE_FACES]{}; 
} extern chunkLookupData;

#endif // _SOURCE_GENERATION_SETTINGS_HDR_
