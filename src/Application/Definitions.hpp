#pragma once
#ifndef _SOURCE_APPLICATION_DEFS_HDR_
#define _SOURCE_APPLICATION_DEFS_HDR_

// glad - OpenGL loader/generator
#include "glad/gladh.h"

// GLFW - Window and input library
#include "glfw/include/GLFW/glfw3.h"

// lodepng - PNG encoder and decoder
#include "lodepng/lodepng.h"

// glm - OpenGL maths library
#define GLM_FORCE_XYZW_ONLY
#include "glm/glm/glm.hpp"
#include "glm/glm/ext/matrix_transform.hpp"
#include "glm/glm/ext/matrix_clip_space.hpp"

// fmt - text formatting library
#define FMT_HEADER_ONLY
#include "fmt/include/fmt/base.h"
#include "fmt/include/fmt/format.h"
#include "fmt/include/fmt/chrono.h"

// Perlin noise generator
#include "World/Generation/Perlin.hpp"

// C libraries
#include <sys/types.h>
#include <sys/stat.h>

// C++ libraries
#include <mutex>
#include <thread>

#include <ctime>
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
	inline void UpdateUBO(GLuint &ubo, const void *data, std::size_t bytes, std::size_t offset = std::size_t{}) noexcept
	{
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		glBufferSubData(GL_UNIFORM_BUFFER, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(bytes), data);
	}
	
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

	template<typename T> static constexpr T pow(T value, int exp) noexcept { return exp == 1 ? value : Math::pow(value, --exp) * value; }
	template<typename T> static constexpr T fract(T floatval) noexcept { return floatval - glm::floor(floatval); }
	template<typename T> static constexpr T roundUpX(T value, T x) noexcept { return value + (value % x); }
	template<typename T> static constexpr T roundDownX(T value, T x) noexcept { return value - (value % x); }

	static constexpr double sqrtImpl(double x, double c, double p = 0.0) noexcept { return c == p ? c : sqrtImpl(x, 0.5 * (c + x / c), c); }
	static constexpr double sqrt(double x) noexcept { return sqrtImpl(x, x); }
	static constexpr double pythagoras(double a, double b) noexcept { return sqrt(a * a + b * b); }
};

// Logging and text formatting
namespace TextFormat
{
	void warn(std::string t, std::string ttl) noexcept;
	void log(std::string t) noexcept;

	std::tm getTime() noexcept;

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
struct ShadersObject
{
	struct Program
	{
		Program(const char* shaderName) noexcept;

		// Compute shaders should include file extension ".comp" within name for identification
		union { GLuint vertex, compute; };
		GLuint program{}, fragment;

		std::string name;

		void Use() const noexcept;
		bool IsCompute() const noexcept;
		GLint GetLocation(const char* name) const noexcept;

		~Program() noexcept;
	};

	// Use a struct to store all programs in one object to easily obtain each
	// individual program without having to write out each one
	struct ShaderPrograms
	{
		Program blocks    = "Blocks",
		        clouds    = "Clouds",
		        inventory = "Inventory",
		        outline   = "Outline",
		        sky       = "Sky",
		        stars     = "Stars",
		        text      = "Text",
		        planets   = "Planets",
		        test      = "Test",
		        border    = "Border";
	} programs;
	
	void InitShaders();
	void EachProgram(std::function<void(Program&)> each);
	static void InitProgram(Program &prog);

	static bool CheckStatus(GLuint id, const std::string &name, bool isShader) noexcept;
	static GLuint CreateShader(const std::string& fileData, const std::string &name, GLenum type);

	void DestroyAll() noexcept;
};

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

struct GameGlobal
{
	void Init() noexcept;

	GLFWwindow *window;
	ShadersObject shaders;
	
	int windowX = 0, windowY = 0;
	int width = 0, height = 0, fwidth = 0, fheight = 0;
	int updateInterval = 1, refreshRate = 60;
	float aspect = 1.0f;

	int numThreads;
	glm::ivec3 workGroupCountMax, workGroupSizeMax;
	int workGroupInvocsMax;
	
	glm::dvec4 testvals = glm::dvec4(1.0);
	bool testbool = false;
	
	bool anyKeyPressed = false;
	bool mainLoopActive = false;
	bool libsInitialized = false;
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

	std::time_t startTime{};
	
	double mouseX = 0.0, mouseY = 0.0, sensitivity = 0.1;
	double deltaTime = 0.016;
	
	std::string currentDirectory;
	std::string texturesFolder, shadersFolder, computesFolder;
	
	WorldNoise noiseGenerators;
	
	ImageInfo blocksTextureInfo;
	ImageInfo textTextureInfo;
	ImageInfo inventoryTextureInfo;
	
	bool holdingCtrl, holdingShift, holdingAlt;
	
	std::unordered_map<int, int> keyboardState;
	
	std::thread *genThreads;

	struct GameConstants { 
		WorldPosition worldDirections[6] = {
			{  1,  0,  0  },	// X+
			{ -1,  0,  0  },	// X-
			{  0,  1,  0  },	// Y+
			{  0, -1,  0  },	// Y-
			{  0,  0,  1  },	// Z+
			{  0,  0, -1  } 	// Z-
		};
		glm::ivec3 worldDirectionsInt[6] = {
			{  1,  0,  0  },	// X+
			{ -1,  0,  0  },	// X-
			{  0,  1,  0  },	// Y+
			{  0, -1,  0  },	// Y-
			{  0,  0,  1  },	// Z+
			{  0,  0, -1  } 	// Z-
		};
		WorldPosition worldDirectionsXZ[4] = {
			{  1,  0,  0  },	// X+
			{ -1,  0,  0  },	// X-
			{  0,  0,  1  },	// Z+
			{  0,  0, -1  } 	// Z-
		};

		std::uint8_t charSizes[95] = {
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

		typedef const char *DirText;
		DirText directionText[6] = { "East", "West", "Up", "Down", "North", "South" };
	} constants{};

	struct PerfTest
	{
		PerfTest(const char* name) noexcept : name(name) {}
		
		std::string name;
		double total = 0.0;
		std::uintmax_t count{};

		void Start() noexcept { currentTime = glfwGetTime(); }
		void End() noexcept { total += glfwGetTime() - currentTime; ++count; }
	private:
		double currentTime;
	};

	struct PerfObject
	{
		PerfTest movement = "Movement",
		         textUpdate = "TextUpdate",
		         frameCols = "FrameCols",
		         renderSort = "RenderSort",
		         invUpdate = "InvUpdate"
		;
	} perfs;

	PerfTest *perfPointers[sizeof(PerfObject) / sizeof(PerfTest)];
	std::size_t perfPointersCount;

	struct GameUBOs { GLuint matricesUBO, timesUBO, coloursUBO, positionsUBO, sizesUBO; } ubos;

	void Cleanup() noexcept;
} extern game;

#endif // _SOURCE_APPLICATION_DEFS_HDR_
