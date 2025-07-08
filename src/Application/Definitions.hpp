#pragma once
#ifndef _SOURCE_APPLICATION_DEFS_HDR_
#define _SOURCE_APPLICATION_DEFS_HDR_

// glad - OpenGL loader/generator
#include "glad/gl.h"

// GLFW - Window and input library
#include "GLFW/glfw3.h"

// glm - OpenGL maths library
#define GLM_FORCE_XYZW_ONLY
#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/matrix_clip_space.hpp"

// lodepng - PNG encoder and decoder
#include "lodepng/lodepng.h"

// fmt - text formatting library
#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "fmt/chrono.h"

// Perlin noise generator
#include "World/Generation/Perlin.hpp"

// C libraries
#include <sys/types.h>
#include <sys/stat.h>

// C++ libraries
#include <mutex>
#include <thread>

#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <iostream>

#include <iterator>
#include <algorithm>
#include <functional>
#include <unordered_map>

typedef std::int64_t PosType; // Switch between 32-bit and 64-bit positioning
typedef glm::vec<3, PosType> WorldPos;
typedef glm::vec<2, PosType> ChunkOffset;

// File system functions
struct FileManager {
	static bool DirectoryExists(std::string path);
	static bool FileExists(std::string path);
	static bool Exists(std::string path);

	static void DeletePath(std::string path);
	static void CreatePath(std::string path);

	class FileError : public std::exception { 
		std::string m_message;
	public:
		FileError(std::string msg) : m_message(msg) {};
		const char *what() const noexcept { return m_message.c_str(); } 
	};
};
// OpenGL function shorcuts
struct OGL 
{
	static GLuint CreateBuffer(GLenum type) noexcept;
	static GLuint CreateVAO(bool bind = true) noexcept;

	static void SetupUBO(GLuint &ubo, GLuint index, std::size_t uboSize) noexcept;
	static void UpdateUBO(GLuint &ubo, GLintptr offset, GLsizeiptr bytes, const void *data) noexcept;

	static std::string GetString(GLenum stringID) noexcept;
};
// Image information
struct OGLImageInfo
{
	std::uint32_t width = 0, height = 0;
	std::vector<unsigned char> data;
};

// Mathematical functions
struct Math
{
	template<typename T, std::size_t N> static std::size_t size(T(&)[N]) { return N; }
	
	static int loopAround(int x, int minInc, int maxExcl) noexcept;

	template<typename T> static T lerp(T a, T b, T t) noexcept { return a + (b - a) * t; }
	template<typename T, glm::qualifier Q> static glm::vec<3, T, Q> lerp(const glm::vec<3, T, Q> &a, const glm::vec<3, T, Q> &b, T t) { 
		return glm::vec<3, T, Q>(lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t)); 
	}

	template<typename T> static T nearestM2(T x, T nearest) noexcept { return (x + nearest - static_cast<T>(1)) & -nearest; }

	static bool isLargeOffset(const WorldPos &pos, PosType threshold = static_cast<PosType>(1000)) noexcept;

	struct WPHash { std::size_t operator()(const WorldPos &vec) const noexcept; };
};

// Logging and text formatting
struct TextFormat
{
	static void warn(std::string t, std::string ttl) noexcept;
	static void log(std::string t, bool nl = true) noexcept;

	static bool stringEndsWith(const std::string &str, const std::string &ending) noexcept;
	static std::string getParentDirectory(const std::string &dir) noexcept;

	static std::int64_t strtoi64(const std::string &numText) noexcept;

	template <typename T> static std::string groupDecimal(T val) noexcept {
		std::string decimalText = fmt::format("{:.3f}", val);
		const int decimalPointIndex = static_cast<int>(decimalText.find('.'));
		for (int i = decimalPointIndex, n = 0, m = val < static_cast<T>(0); i > m; --i) if (!(++n % 4)) { decimalText.insert(i, 1, ','); n = 1; }
		return decimalText;
	}
};

// Vertex and fragment shader loader as well and disk reading
class Shader
{
public:
	void InitShader();
	enum class ShaderID : int { Blocks, Clouds, Inventory, Outline, Sky, Stars, Text, SunMoon, Vec3, Border, MAX };

	GLint GetLocation(ShaderID id, const char *name) noexcept;
	GLuint IDFromShaderName(ShaderID id) const noexcept;
	void UseShader(ShaderID id) const noexcept;

	static std::string ReadFileFromDisk(const std::string &filename);
	static void LoadImage(OGLImageInfo &info, const char *filename, bool border, int filterParam, bool mipmap);

	~Shader() noexcept;
private:
	GLuint m_programs[static_cast<int>(ShaderID::MAX)] {};
};

// Combined perlin noise struct
struct WorldNoise
{
	WorldNoise() noexcept = default;
	WorldNoise(WorldPerlin::NoiseSpline *splines);
	WorldNoise(WorldPerlin::NoiseSpline *splines, std::int64_t *seeds);
	
	enum NoiseEnums : int { Continent, Flat, Depth, Temp, Humidity, MAX };

	WorldPerlin continentalness;
	WorldPerlin flatness;
	WorldPerlin depth;
	WorldPerlin temperature;
	WorldPerlin humidity;
};

// Globals
struct GameGlobalsObject
{
	GLFWwindow *window = nullptr;

	std::vector<std::thread> standaloneThreads;
	WorldNoise noiseGenerators;

	int windowX = 0, windowY = 0;
	int width = 0, height = 0, fwidth = 0, fheight = 0;
	int updateInterval = 1;
	float aspect = 1.0f;

	glm::dvec4 testvals { 1.0, 1.0, 1.0, 1.0 };
	bool testbool = false;

	Shader shader;

	bool anyKeyPressed = false;
	bool mainLoopActive = false;
	bool focusChanged = true;
	bool chatting = false;
	bool ignoreChatStart = false;
	bool isChatCommand = false;
	bool minimized = false;
	bool isServer = false;
	bool noGeneration = false;
	bool showGUI = true;
	bool maxFPS = false;
	bool wireframe = false;
	bool fullscreen = false;
	bool debugText = true;
	bool chunkBorders = false;

	double tickSpeed = 1.0;
	double tickedDeltaTime = 0.016;
	double daySeconds = 0.0;

	std::int64_t worldDay = 0;

	double mouseX = 0.0, mouseY = 0.0, sensitivity = 0.1;
	double deltaTime = 0.016;
	
	std::string currentDirectory;
	std::string resourcesFolder;

	OGLImageInfo blocksTextureInfo;
	OGLImageInfo textTextureInfo;
	OGLImageInfo inventoryTextureInfo;

	bool holdingCtrl, holdingShift, holdingAlt;

	std::unordered_map<int, int> keyboardState;

	struct ClassConstants { 
		const WorldPos worldDirections[6] = {
			{  1,  0,  0  },	// X+
			{ -1,  0,  0  },	// X-
			{  0,  1,  0  },	// Y+
			{  0, -1,  0  },	// Y-
			{  0,  0,  1  },	// Z+
			{  0,  0, -1  } 	// Z-
		};
		const glm::ivec3 worldDirectionsInt[6] = {
			{  1,  0,  0  },	// X+
			{ -1,  0,  0  },	// X-
			{  0,  1,  0  },	// Y+
			{  0, -1,  0  },	// Y-
			{  0,  0,  1  },	// Z+
			{  0,  0, -1  } 	// Z-
		};
		const WorldPos worldDirectionsXZ[4] = {
			{  1,  0,  0  },	// X+
			{ -1,  0,  0  },	// X-
			{  0,  0,  1  },	// Z+
			{  0,  0, -1  } 	// Z-
		};

		const std::uint8_t charSizes[95] = {
		//	!  "  #  $  %  &  '  (  )  *  +  ,  -  .  /
			1, 3, 6, 5, 9, 6, 1, 2, 2, 5, 5, 2, 5, 1, 3,
		//	0  1  2  3  4  5  6  7  8  9
			5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
		//	:  ;  <  =  >  ?  @
			1, 2, 4, 5, 4, 5, 6,
		//	A  B  C  D  E  F  G  H  I  J  K  L  M
			5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
		//	N  O  P  Q  R  S  T  U  V  W  X  Y  Z
			5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
		//	[  \  ]  ^  _  `
			2, 3, 2, 5, 5, 2,
		//	a  b  c  d  e  f  g  h  i  j  k  l  m
			4, 4, 4, 4, 4, 4, 4, 4, 1, 3, 4, 3, 5,
		//	n  o  p  q  r  s  t  u  v  w  x  y  z
			4, 4, 4, 4, 4, 4, 3, 4, 5, 5, 5, 4, 4,
		//	{  |  }  ~
			3, 1, 3, 6,
		//	Background character (custom)
			1
		};
	} constants{};
} extern game;

#endif // _SOURCE_APPLICATION_DEFS_HDR_
