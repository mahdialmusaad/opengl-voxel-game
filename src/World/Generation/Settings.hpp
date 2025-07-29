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
		int light,
		int strength,
		renderLookupDefinition renderFunc
	) noexcept :
		id(static_cast<int>(oid)),
		name(name),
		textures{ t0, t1, t2, t3, t4, t5 },
		emission(static_cast<std::uint8_t>(light)),
		strength(static_cast<std::uint8_t>(strength)),
		hasTransparency(hasTransparency),
		isSolid(isSolid),
		notObscuredBy(renderFunc) {
	}

	const int id;
	const char *name;
	const std::uint8_t textures[6], emission, strength;
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
		DefaultTex = TextureIDTypeof{},
	};
	enum BlockDataBoolean : bool {
		Transp = true,
		Opaque = false,
		YSolid = true,
		NSolid = false
	};
	enum LightStages {
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
	{ 
		ObjectID::Air, "Air",
		DefaultTex, DefaultTex, DefaultTex, DefaultTex, DefaultTex, DefaultTex, 
		Transp, NSolid, LightN, 0, R_Never
	},
	{ 
		ObjectID::Grass, "Grass",
		GrassSide, GrassSide, GrassTop, Dirt, GrassSide, GrassSide, 
		Opaque, YSolid, LightN, 8, R_Default
	},
	{
		ObjectID::Dirt, "Dirt",
		Dirt, Dirt, Dirt, Dirt, Dirt, Dirt,
		Opaque, YSolid, LightN, 7, R_Default
	},
	{
		ObjectID::Stone, "Stone",
		Stone, Stone, Stone, Stone, Stone, Stone,
		Opaque, YSolid, LightN, 30, R_Default
	},
	{
		ObjectID::Sand, "Sand",
		Sand, Sand, Sand, Sand, Sand, Sand,
		Opaque, YSolid, LightN, 6, R_Default
	},
	{
		ObjectID::Water, "Water",
		Water, Water, Water, Water, Water, Water,
		Transp, NSolid, LightN, 0, R_HideSelf
	},
	{
		ObjectID::Log, "Log",
		LogSide, LogSide, LogInner, LogInner, LogSide, LogSide,
		Opaque, YSolid, LightN, 12, R_Default
	},
	{
		ObjectID::Leaves, "Leaves",
		Leaves, Leaves, Leaves, Leaves, Leaves, Leaves,
		Transp, YSolid, LightN, 3, R_Default
	},
	{
		ObjectID::Planks, "Planks",
		Planks, Planks, Planks, Planks, Planks, Planks,
		Opaque, YSolid, LightN, 10, R_Default
	},
	};
};

// Game settings
namespace ChunkValues
{
	constexpr int size = 16; // The side length of a chunk (cube).
	constexpr int worldHeight = 256; // The maximum block height in the world (must be a multiple of the chunk size)
	
	// Chunk generation settings (editable)
	constexpr int baseDirtHeight = 3; // Amount of dirt blocks between surface and stone.
	constexpr int waterMaxHeight = 80; // Maximum Y position of water.
	constexpr int treeSpawnChance = 100; // The chance for a grass block to have a tree.
	// Settings/values to do with noise can be found in the 'perlin' file.
	
	// Calculation shortcuts - do not change
	constexpr int heightCount = worldHeight / size;
	constexpr int sizeLess = size - 1;
	constexpr int sizeSquared = size * size;
	const double sqrtSize = Math::sqrt(size);
	constexpr int maxHeight = size * heightCount;

	// Use template recursion to get no. of bits needed to store an integer at runtime
	template<int X> struct bitsStore { enum { value = bitsStore<(X >> 1)>::value + 1 }; };
	template<> struct bitsStore<0> { enum { value = 0 }; };
	constexpr int sizeBits = 5; // This value must be reflected in the block shader.

	constexpr std::int32_t blocksAmount = size * size * size;
	constexpr std::int32_t uniqueFaces = blocksAmount * 6;

	template<typename T> WorldBlockData::WBD GetBlockData(T blockID) noexcept { return WorldBlockData_DEF::BlockIDData[static_cast<int>(blockID)]; }

	PosType ToWorld(double a) noexcept;
	template<typename T> PosType ToWorld(T a) noexcept { return static_cast<PosType>(a); }
	template<typename T, glm::qualifier Q> WorldPosition ToWorld(const glm::vec<3, T, Q> &vec) noexcept {
		return { ToWorld(vec.x), ToWorld(vec.y), ToWorld(vec.z) };
	}
	
	template<typename T> PosType WorldToOffset(T x) noexcept {
		const PosType v = ToWorld(x);
		const PosType a = v < PosType{} ? v - static_cast<PosType>(sizeLess) : v;
		return (a - (a % static_cast<PosType>(size))) / static_cast<PosType>(size);
	}
	template<typename T, glm::qualifier Q> WorldPosition WorldToOffset(const glm::vec<3, T, Q> &pos) noexcept {
		return { WorldToOffset(pos.x), WorldToOffset(pos.y), WorldToOffset(pos.z) };
	}
	
	template<typename T> int WorldToLocal(T x) noexcept {
		return static_cast<int>(static_cast<PosType>(x) - (static_cast<PosType>(size) * WorldToOffset(x)));
	}
	template<typename T, glm::qualifier Q> glm::ivec3 WorldToLocal(const glm::vec<3, T, Q> &pos) noexcept {
		return { WorldToLocal(pos.x), WorldToLocal(pos.y), WorldToLocal(pos.z) };
	}
	
	bool IsOnCorner(const glm::ivec3 &pos, WorldDirection dir) noexcept;

	struct BlockArray {
		ObjectID blocks[size][size][size];
		template<typename T, glm::qualifier Q> ObjectID at(glm::vec<3, T, Q> v) const noexcept { return blocks[v.x][v.y][v.z]; }
		ObjectID &atref(glm::ivec3 v) noexcept { return blocks[v.x][v.y][v.z]; }
	} const emptyChunk{};

	static_assert(!(worldHeight % size), "The world height must be a multiple of the chunk size.");
};

struct ChunkLookupData
{
	glm::i8vec3 pos;
	std::uint8_t index;
	static void CalculateLookupData(ChunkLookupData *lookupData) noexcept;
} extern chunkLookupData[ChunkValues::uniqueFaces];

// Ranges for certain values across the game
namespace ValueLimits
{
	template<typename T> struct LVec { T min, max; };
	template<typename T> using LMS = std::numeric_limits<T>;
	template<typename T> void ClampAdd(T &val, T add, const LVec<T> &lvec) { val = glm::clamp(val + add, lvec.min, lvec.max); };

	constexpr double VLradian = 0.01745329251994329576923690768489;
	constexpr LVec<double> fovLimit  = { 5.0 * VLradian, 160.0 * VLradian };
	constexpr LVec<int> renderLimit  = { 4, LMS<int>::max() };
	constexpr LVec<double> tickLimit = { -200.0, 200.0 };
}

#endif // _SOURCE_GENERATION_SETTINGS_HDR_
