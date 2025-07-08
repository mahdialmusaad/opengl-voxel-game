#pragma once

#ifndef _SOURCE_GENERATION_SETTINGS_HDR_
#define _SOURCE_GENERATION_SETTINGS_HDR_

#include "Application/Definitions.hpp"
#include "Rendering/Frustum.hpp"

enum WorldDirection : int
{
	WldDir_None  = -1, // none
	WldDir_Right =  0, // positive X
	WldDir_Left  =  1, // negative X
	WldDir_Up    =  2, // positive Y
	WldDir_Down  =  3, // negative Y
	WldDir_Front =  4, // positive Z
	WldDir_Back  =  5, // negative Z
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
	typedef const WorldBlockData &WBD;
	typedef bool (*renderLookupDefinition)(WBD original, WBD target);

	constexpr WorldBlockData(
		ObjectID oid,
		const char *name,
		TextureIDTypeof t0, TextureIDTypeof t1, TextureIDTypeof t2, TextureIDTypeof t3, TextureIDTypeof t4, TextureIDTypeof t5,
		bool hasTransparency,
		bool isSolid,
		std::int8_t strength,
		std::uint8_t light,
		renderLookupDefinition renderFunc
	) noexcept :
		id(static_cast<int>(oid)),
		name(name),
		textures{ t0, t1, t2, t3, t4, t5 },
		lightEmission(light),
		strength(static_cast<std::uint8_t>(strength)),
		hasTransparency(hasTransparency),
		isSolid(isSolid),
		notObscuredBy(*renderFunc) {
	}

	const int id;
	const char *name;
	const std::uint8_t textures[6], lightEmission, strength;
	const bool hasTransparency, isSolid;
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
	enum BlockDataBoolean : bool {
		Transp = true,
		Opaque = false,
		YSolid = true,
		NSolid = false
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
	typedef WorldBlockData::WBD WBD;
	constexpr bool R_Never(WBD, WBD) { return false; }
	constexpr bool R_Always(WBD, WBD) { return true; }
	constexpr bool R_HideSelf(WBD original, WBD target) { return original.id != target.id && target.hasTransparency; }
	constexpr bool R_Default(WBD, WBD target) { return target.hasTransparency; }

	// All blocks and their properties
	constexpr WorldBlockData BlockIDData[static_cast<int>(ObjectID::NumUnique)] = {
	// ID, tex0, tex1, tex2, tex3, tex4, tex5, transparency, solid, light, strength, function
		{ 
			ObjectID::Air, "Air",
			DefaultTex, DefaultTex, DefaultTex, DefaultTex, DefaultTex, DefaultTex, 
			Transp, NSolid, 0, LightN, R_Never
		},
		{ 
			ObjectID::Grass, "Grass",
			GrassSide, GrassSide, GrassTop, Dirt, GrassSide, GrassSide, 
			Opaque, YSolid, 8, LightN, R_Default
		},
		{
			ObjectID::Dirt, "Dirt",
			Dirt, Dirt, Dirt, Dirt, Dirt, Dirt,
			Opaque, YSolid, 7, LightN, R_Default
		},
		{
			ObjectID::Stone, "Stone",
			Stone, Stone, Stone, Stone, Stone, Stone,
			Opaque, YSolid, 30, LightN, R_Default
		},
		{
			ObjectID::Sand, "Sand",
			Sand, Sand, Sand, Sand, Sand, Sand,
			Opaque, YSolid, 6, LightN, R_Default
		},
		{
			ObjectID::Water, "Water",
			Water, Water, Water, Water, Water, Water,
			Transp, NSolid, 0, LightN, R_HideSelf
		},
		{
			ObjectID::Log, "Log",
			LogSide, LogSide, LogInner, LogInner, LogSide, LogSide,
			Opaque, YSolid, 12, LightN, R_Default
		},
		{
			ObjectID::Leaves, "Leaves",
			Leaves, Leaves, Leaves, Leaves, Leaves, Leaves,
			Transp, YSolid, 3, LightN, R_Default
		},
		{
			ObjectID::Planks, "Planks",
			Planks, Planks, Planks, Planks, Planks, Planks,
			Opaque, YSolid, 10, LightN, R_Default
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
	constexpr int TREE_GEN_CHANCE = 100;
	
	// Calculation shortcuts - do not change
	constexpr int CHUNK_SIZE_SQUARED = CHUNK_SIZE * CHUNK_SIZE;
	constexpr int MAX_WORLD_HEIGHT = CHUNK_SIZE * HEIGHT_COUNT;

	constexpr std::int32_t CHUNK_BLOCKS_AMOUNT = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
	constexpr std::int32_t CHUNK_UNIQUE_FACES = CHUNK_BLOCKS_AMOUNT * 6;

	constexpr int MAX_WORLD_HEIGHT_INDEX = MAX_WORLD_HEIGHT - 1;
	constexpr float NOISE_MULTIPLIER = static_cast<float>(MAX_WORLD_HEIGHT) * 0.5f;

	constexpr int CHUNK_SIZE_HALF = CHUNK_SIZE / 2;

	constexpr int CHUNK_SIZE_M1 = CHUNK_SIZE - 1;
	constexpr float CHUNK_SIZE_FLT = static_cast<float>(CHUNK_SIZE);
	constexpr double CHUNK_SIZE_DBL = static_cast<double>(CHUNK_SIZE);

	constexpr float CHUNK_SIZE_RECIP = 1.0f / CHUNK_SIZE_FLT;
	constexpr double CHUNK_SIZE_RECIP_DBL = 1.0 / CHUNK_SIZE_DBL;

	template<typename T> WorldBlockData::WBD GetBlockData(T blockID) noexcept { return WorldBlockData_DEF::BlockIDData[static_cast<int>(blockID)]; }

	PosType ToWorld(double a) noexcept;
	template<typename T> PosType ToWorld(T a) noexcept { return static_cast<PosType>(a); }
	template<typename T, glm::qualifier Q> WorldPos ToWorld(const glm::vec<3, T, Q> &vec) noexcept { return { ToWorld(vec.x), ToWorld(vec.y), ToWorld(vec.z) }; }
	
	template<typename T> PosType WorldToOffset(T x) noexcept {
		const PosType v = ToWorld(x);
		const PosType a = v < static_cast<PosType>(0) ? v - static_cast<PosType>(CHUNK_SIZE_M1) : v;
		return (a - (a % static_cast<PosType>(CHUNK_SIZE))) / static_cast<PosType>(CHUNK_SIZE);
	}
	template<typename T, glm::qualifier Q> WorldPos WorldToOffset(const glm::vec<3, T, Q> &pos) noexcept { return { WorldToOffset(pos.x), WorldToOffset(pos.y), WorldToOffset(pos.z) }; }
	
	template<typename T> int WorldToLocal(T x) noexcept { return static_cast<int>(static_cast<PosType>(x) - (static_cast<PosType>(CHUNK_SIZE) * WorldToOffset(x))); }
	template<typename T, glm::qualifier Q> glm::ivec3 WorldToLocal(const glm::vec<3, T, Q> &pos) noexcept { return { WorldToLocal(pos.x), WorldToLocal(pos.y), WorldToLocal(pos.z) }; }
	
	bool IsOnCorner(const glm::ivec3 &pos, WorldDirection dir) noexcept;
	bool ChunkOnFrustum(const CameraFrustum &frustum, glm::vec3 center) noexcept;

	struct BlockArray {
		typedef ObjectID (InnerArrayDef)[CHUNK_SIZE];
		typedef InnerArrayDef (OuterArrayDef)[CHUNK_SIZE];
		typedef OuterArrayDef (ArrayDef)[CHUNK_SIZE];

		ArrayDef blocks{};
		template<typename T, glm::qualifier Q> ObjectID at(glm::vec<3, T, Q> v) const noexcept { return blocks[v.x][v.y][v.z]; }
		ObjectID &atref(glm::ivec3 v) noexcept { return blocks[v.x][v.y][v.z]; }
	} const emptyChunk{};
};

struct ChunkLookupData {
	glm::i8vec3 pos;
	std::uint8_t index;
	static void CalculateLookupData(ChunkLookupData (&lookupData)[ChunkSettings::CHUNK_UNIQUE_FACES]) noexcept;
} extern chunkLookupData[ChunkSettings::CHUNK_UNIQUE_FACES];

#endif // _SOURCE_GENERATION_SETTINGS_HDR_
