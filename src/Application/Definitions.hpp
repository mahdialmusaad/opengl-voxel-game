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
#include "fmt/base.h"
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
typedef glm::vec<3, PosType> WorldPosition;
typedef glm::vec<2, PosType> WorldXZPosition;

// OpenGL function shorcuts
namespace OGL 
{
	GLuint CreateBuffer(GLenum type) noexcept;
	GLuint CreateVAO(bool bind = true) noexcept;
	
	void SetupUBO(GLuint &ubo, GLuint index, std::size_t uboSize) noexcept;
	void UpdateUBO(GLuint &ubo, const void *data, std::size_t bytes, std::size_t offset = std::size_t{}) noexcept;
	
	std::string GetString(GLenum stringID) noexcept;
};
// Image information
struct ImageInfo
{
	std::uint32_t width{}, height{};
	std::vector<unsigned char> data;

	struct Mipmap {
		Mipmap(GLint mipParam = 0, float bias = 0.0f) noexcept : mipmapParam(mipParam), bias(bias) {}
		GLint mipmapParam; float bias;
	};
	struct Format {
		Format(GLint OGLformat = GL_RGBA, LodePNGColorType imgFormat = LodePNGColorType::LCT_RGBA, unsigned bitdepth = 8u, GLint wrap = GL_REPEAT) noexcept
			: wrapParam(wrap), OGLFormat(OGLformat), imgFormat(imgFormat), bitDepth(bitdepth) {}
		GLint wrapParam, OGLFormat;
		LodePNGColorType imgFormat;
		unsigned bitDepth;
	};
};

// Mathematical functions
struct Math
{
	struct WPHash { 
		std::size_t operator()(const WorldPosition &vec) const noexcept { 
			constexpr PosType one = static_cast<PosType>(1);
			return vec.x ^ ( ( (vec.y << one) ^ (vec.z << one) ) >> one);
		}
	};

	template<typename T, std::size_t N> static constexpr std::size_t size(T(&)[N]) { return N; }
	template<typename T, typename D> static constexpr std::size_t size() { return sizeof(T) / sizeof(D); }
	
	static int loop(int x, int minInc, int maxExcl) noexcept {
		return minInc + ((maxExcl + x) % maxExcl);
	}
	
	template<typename T> static T lerp(T a, T b, T t) noexcept { return a + (b - a) * t; }
	template<typename T, glm::qualifier Q> glm::vec<3, T, Q> static lerp(const glm::vec<3, T, Q> &a, const glm::vec<3, T, Q> &b, T t) { 
		return glm::vec<3, T, Q>(lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t)); 
	}
	
	template<typename T, glm::qualifier Q> static T largestUnsignedAxis(const glm::vec<3, T, Q> &vec) noexcept {
		return glm::max(glm::max(glm::abs(vec.x), glm::abs(vec.y)), glm::abs(vec.z));
	}
	static bool isLargeOffset(const WorldPosition &off, PosType threshold = static_cast<PosType>(1000)) noexcept {
		return largestUnsignedAxis(off) >= threshold;
	}
	
	static constexpr double sqrtImpl(double x, double c, double p = 0.0) { return c == p ? c : sqrtImpl(x, 0.5 * (c + x / c), c); }
	static constexpr double sqrt(double x) { return sqrtImpl(x, x); }
	static constexpr double pythagoras(double a, double b) { return sqrt(a * a + b * b); }
};

// Logging and text formatting
namespace TextFormat
{
	void warn(std::string t, std::string ttl) noexcept;
	void log(std::string t, bool nl = true) noexcept;

	int numChar(const std::string &str, const char c) noexcept;
	bool stringEndsWith(const std::string &str, const std::string &ending) noexcept;
	void strsplit(std::vector<std::string> &vec, const std::string &str, const char sp) noexcept;
	std::size_t findNthOf(const std::string &str, const char find, int nth) noexcept;
	std::intmax_t strtoimax(const std::string &numText);
	
	template <typename T> std::string groupDecimal(T val) noexcept
	{
		std::string decimalText = fmt::format("{:.3f}", val);
		const int decimalPointIndex = static_cast<int>(decimalText.find('.'));
		for (int i = decimalPointIndex, n = 0, m = val < T{}; i > m; --i) if (!(++n % 4)) { decimalText.insert(i, 1, ','); n = 1; }
		return decimalText;
	}
};

// Vertex and fragment shader loader
struct Shader
{
	struct Program
	{
		Program(const char* shaderName) noexcept;
		GLuint program;
		std::string name;
	};

	// Use a struct to store all programs in one object to easily obtain each
	// individual program without having to write out each one
	struct ShaderPrograms
	{
		Program blocks    = "blocks",
		        clouds    = "clouds",
		        inventory = "inventory",
		        outline   = "outline",
		        sky       = "sky",
		        stars     = "stars",
		        text      = "text",
		        planets   = "planets",
		        test      = "test",
		        border    = "border";
	} programs;
	
	void InitShaders();
	void EachProgram(std::function<void(Program&)> each);

	void UseProgram(Program &program) const noexcept;
	GLint GetLocation(Program &program, const char* name) const noexcept;

	static bool CheckShaderStatus(GLuint shader, const std::string &name) noexcept;
	static GLuint CreateShader(const std::string& fileData, const std::string &name, bool isVertex) noexcept;

	static void InitProgramFile(Program &prog, const std::string &vertexData, const std::string &fragmentData);
	static void InitProgram(Program &prog);

	~Shader() noexcept;
} extern shader;

// File system functions
namespace FileManager
{
	void ReadFile(const std::string &filename, std::string &contents, bool log, bool throwing);
	void LoadImage(ImageInfo &info, const std::string &filename, const ImageInfo::Format &formatInfo = {}, const ImageInfo::Mipmap &mipmapInfo = {});

	bool DirectoryExists(std::string path);
	void GetParentDirectory(std::string &directory) noexcept;

	bool FileExists(std::string path);
	bool Exists(std::string path);

	void DeletePath(std::string path);
	void CreatePath(std::string path);

	class FileError : public std::exception { 
		std::string m_message;
	public:
		FileError(std::string msg) : m_message(msg) {};
		const char *what() const noexcept { return m_message.c_str(); } 
	};
};

// Combined perlin noise struct
struct WorldNoise
{
	WorldNoise() noexcept = default;
	WorldNoise(WorldPerlin::NoiseSpline *splines);
	WorldNoise(WorldPerlin::NoiseSpline *splines, std::int64_t *seeds);
	
	enum NoiseEnums : int { Elevation, Flat, Depth, Temperature, Humidity, MAX };

	WorldPerlin elevation;
	WorldPerlin flatness;
	WorldPerlin depth;
	WorldPerlin temperature;
	WorldPerlin humidity;
};

struct GameGlobalsObject
{
	GLFWwindow *window = nullptr;
	
	int windowX = 0, windowY = 0;
	int width = 0, height = 0, fwidth = 0, fheight = 0;
	int updateInterval = 1, refreshRate = 60;
	float aspect = 1.0f;

	int threads;
	
	glm::dvec4 testvals = glm::dvec4(1.0);
	bool testbool = false;
	
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
	bool vsyncFPS = true;
	bool wireframe = false;
	bool fullscreen = false;
	bool debugText = true;
	bool chunkBorders = false;
	bool hideFog = false;
	
	double tickSpeed = 1.0;
	double tickedDeltaTime = 0.016;
	double daySeconds = 0.0;
	
	std::int64_t worldDay{};
	std::uint64_t gameFrame{};
	
	double mouseX = 0.0, mouseY = 0.0, sensitivity = 0.1;
	double deltaTime = 0.016;
	
	std::string currentDirectory;
	std::string resourcesFolder;
	
	WorldNoise noiseGenerators;
	
	ImageInfo blocksTextureInfo;
	ImageInfo textTextureInfo;
	ImageInfo inventoryTextureInfo;
	
	bool holdingCtrl, holdingShift, holdingAlt;
	
	std::unordered_map<int, int> keyboardState;

	std::mutex chunkCreationMutex;

	struct GameConstants { 
		const WorldPosition worldDirections[6] = {
			{  1,  0,  0  },	// X+
			{ -1,  0,  0  },	// X-
			{  0,  1,  0  },	// Y+
			{  0, -1,  0  },	// Y-
			{  0,  0,  1  },	// Z+
			{  0,  0, -1  } 	// Z-
		};
		const WorldPosition worldDirectionsXZ[4] = {
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

		const std::string directionText[6] = { "East", "West", "Up", "Down", "North", "South" };
	} constants{};

	struct PerfObject
	{
		struct PerfTestData
		{
			PerfTestData(const char* name) noexcept : name(name) {}
			
			std::string name;
			double total = 0.0;
			std::uintmax_t count{};

			void Start() noexcept { currentTime = glfwGetTime(); }
			void End() noexcept { total += glfwGetTime() - currentTime; ++count; }
		private:
			double currentTime;
		} culling = "Culling",
		  movement = "Movement",
		  chunkCalc = "TerrainCalc",
		  generation = "Generation",
		  textUpdate = "TextUpdate",
		  frameVals = "FrameCols",
		  uboUpdate = "UBOUpdate"
		;
	} perfs;

	typedef PerfObject::PerfTestData PerfData;
	PerfData *perfPointers[sizeof(PerfObject) / sizeof(PerfData)];
	const int perfPointersCount = Math::size(perfPointers);
	
	GameGlobalsObject() noexcept {
		PerfData* perfPtrs = reinterpret_cast<PerfData*>(&perfs);
		for (int i = 0; i < perfPointersCount; ++i) perfPointers[i] = &perfPtrs[i];
	}
} extern game;

typedef GameGlobalsObject::PerfData *GamePerfTest;

#endif // _SOURCE_APPLICATION_DEFS_HDR_
