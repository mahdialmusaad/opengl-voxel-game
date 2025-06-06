#pragma once
#ifndef _SOURCE_GENERATION_SETTINGS_HDR_
#define _SOURCE_GENERATION_SETTINGS_HDR_

#include "Globals/Definitions.hpp"
#include "Rendering/Frustum.hpp"

enum class WorldDirection : std::int8_t
{
	None	= -1, // none
	Right	= 0, // positive X
	Left	= 1, // negative X
	Up		= 2, // positive Y
	Down	= 3, // negative Y
	Front	= 4, // positive Z
	Back	= 5, // negative Z
};

enum WorldDirectionInt
{
	IWorldDir_None 	= -1, // none
	IWorldDir_Right = 0, // positive X
	IWorldDir_Left 	= 1, // negative X
	IWorldDir_Up 	= 2, // positive Y
	IWorldDir_Down 	= 3, // negative Y
	IWorldDir_Front = 4, // positive Z
	IWorldDir_Back 	= 5, // negative Z
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
			GrassSide, GrassSide, GrassTop, Dirt, GrassSide, GrassSide, 
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
			LogSide, LogSide, LogInner, LogInner, LogSide, LogSide,
			Opaque, Solid, Irreplaceable, LightN, R_Default
		},
		{
			ObjectID::Leaves, "Leaves",
			Leaves, Leaves, Leaves, Leaves, Leaves, Leaves,
			Transparency, Solid, Replaceable, LightN, R_Default
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

	// Chunk generation settings (editable)
	constexpr int CHUNK_BASE_DIRT_HEIGHT = HEIGHT_COUNT / 2;
	constexpr int CHUNK_WATER_HEIGHT = CHUNK_SIZE * HEIGHT_COUNT / 4;
	constexpr int CHUNK_MOUNTAIN_HEIGHT = CHUNK_WATER_HEIGHT * 3;
	constexpr int CHUNK_LAND_MINIMUM_HEIGHT = CHUNK_WATER_HEIGHT / 2;
	constexpr double NOISE_STEP = 0.01;
	
	// Calculation shortcuts - do not change
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
	constexpr int CHUNK_SIZE_POW = ([](int x){ return x; })(5);
	constexpr int CHUNK_SIZE_POW2 = CHUNK_SIZE_POW * 2;

	constexpr float CHUNK_SIZE_RECIP = 1.0f / CHUNK_SIZE_FLT;
	constexpr double CHUNK_SIZE_RECIP_DBL = 1.0 / CHUNK_SIZE_DBL;

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
		{  1,  0,  0  },	// X+
		{ -1,  0,  0  },	// X-
		{  0,  1,  0  },	// Y+
		{  0, -1,  0  },	// Y-
		{  0,  0,  1  },	// Z+
		{  0,  0, -1  } 	// Z-
	};

	constexpr glm::ivec3 worldDirectionsInt[6] = {
		{  1,  0,  0  },	// X+
		{ -1,  0,  0  },	// X-
		{  0,  1,  0  },	// Y+
		{  0, -1,  0  },	// Y-
		{  0,  0,  1  },	// Z+
		{  0,  0, -1  } 	// Z-
	};

	constexpr WorldPos worldDirectionsXZ[4] = {
		{  1,  0,  0  },	// X+
		{ -1,  0,  0  },	// X-
		{  0,  0,  1  },	// Z+
		{  0,  0, -1  } 	// Z-
	};

	PosType WorldPositionToOffset(PosType x) noexcept;
	WorldPos WorldPositionToOffset(PosType x, PosType y, PosType z) noexcept;

	int WorldToLocalPosition(PosType x) noexcept;
	glm::ivec3 WorldToLocalPosition(PosType x, PosType y, PosType z) noexcept;

	WorldPos GetChunkCorner(const WorldPos& offset) noexcept;
	WorldPos GetChunkCenter(const WorldPos& offset) noexcept;
	bool ChunkOnFrustum(CameraFrustum& frustum, glm::vec3 center) noexcept;

	enum class ChunkBlockValueType { Full, Compressed, Air };

	typedef ObjectID(blockArray[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE]);
	constexpr blockArray emptyChunk{};
	constexpr ObjectID lonelyAirBlock = ObjectID::Air;

	struct ChunkBlockValue
	{
		virtual ObjectID GetBlock(int, int, int) const noexcept = 0;
		virtual void SetBlock(int, int, int, ObjectID) noexcept = 0;

		virtual ChunkBlockValueType GetChunkBlockType() const noexcept = 0;
		virtual ~ChunkBlockValue() noexcept = default;
	};

	struct ChunkBlockValueFull : ChunkBlockValue
	{
		blockArray chunkBlocks{};

		ObjectID GetBlock(int x, int y, int z) const noexcept override { return chunkBlocks[x][y][z]; }
		void SetBlock(int x, int y, int z, ObjectID block) noexcept override { chunkBlocks[x][y][z] = block; }

		ChunkBlockValueType GetChunkBlockType() const noexcept override { return ChunkBlockValueType::Full; }
		~ChunkBlockValueFull() noexcept = default;
	};

	struct ChunkBlockAir : ChunkBlockValue
	{
		ObjectID GetBlock(int, int, int) const noexcept override { return ObjectID::Air; }
		void SetBlock(int, int, int, ObjectID) noexcept override { TextFormat::log("Single chunk requires conversion for set"); }

		ChunkBlockValueType GetChunkBlockType() const noexcept override { return ChunkBlockValueType::Air; }
		~ChunkBlockAir() noexcept = default;
	};

	bool IsAirChunk(ChunkBlockValue* chunkBlocks) noexcept;
	ChunkBlockValueFull* GetFullBlockArray(ChunkBlockValue* chunkBlocks) noexcept;
};

struct ChunkLookupTable
{
	ChunkLookupTable() noexcept
	{
		// Precalculated results for chunk calculation - use to check which block is 
		// next to another and in which 'nearby chunk' (if it happens to be outside)
		std::int32_t lookupIndex = 0;

		for (std::uint8_t face = 0; face < 6u; ++face) {
			const glm::ivec3 nextDirection = ChunkSettings::worldDirectionsInt[face];
			for (int x = 0; x < ChunkSettings::CHUNK_SIZE; ++x) {
				const int nextX = x + nextDirection.x;
				const bool xOverflowing = nextX < 0 || nextX >= ChunkSettings::CHUNK_SIZE;

				for (int y = 0; y < ChunkSettings::CHUNK_SIZE; ++y) {
					const int nextY = y + nextDirection.y;
					const bool yOverflowing = nextY < 0 || nextY >= ChunkSettings::CHUNK_SIZE;
					const bool xyOverflowing = xOverflowing || yOverflowing;

					for (int z = 0; z < ChunkSettings::CHUNK_SIZE; ++z) {
						const int nextZ = z + nextDirection.z;
						const bool zOverflowing = nextZ < 0 || nextZ >= ChunkSettings::CHUNK_SIZE;
						ChunkLookupData& data = lookupData[lookupIndex++];

						data.nextPos = glm::i8vec3(
							Math::loopAround(nextX, 0, ChunkSettings::CHUNK_SIZE),
							Math::loopAround(nextY, 0, ChunkSettings::CHUNK_SIZE),
							Math::loopAround(nextZ, 0, ChunkSettings::CHUNK_SIZE)
						);
						data.nearbyIndex = (xyOverflowing || zOverflowing) ? face : static_cast<std::uint8_t>(6u);
					}
				}
			}
		}
	}

	struct ChunkLookupData {
		glm::i8vec3 nextPos;
		std::uint8_t nearbyIndex;
	};

	ChunkLookupData lookupData[ChunkSettings::CHUNK_UNIQUE_FACES]{}; 
} extern chunkLookupData;

#endif // _SOURCE_GENERATION_SETTINGS_HDR_
