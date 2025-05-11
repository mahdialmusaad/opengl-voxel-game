#pragma once
#ifndef _SOURCE_UTILITY_UTILS_HEADER_
#define _SOURCE_UTILITY_UTILS_HEADER_

#define GAME_SINGLE_THREAD

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

// C++ standard libraries
#include <mutex>
#include <thread>

#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <iostream>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <unordered_map>

// Easily reverse a bool without having to type the name twice, e.g. longboolname = !longboolname
constexpr bool b_not(bool& x) noexcept
{
	x = !x;
	return x;
}

typedef std::int64_t PosType; // Switch between 32-bit and 64-bit positioning
typedef glm::vec<3, PosType> WorldPos;
typedef glm::vec<2, PosType> ChunkOffset;

// OpenGL function shorcuts
struct OGL 
{
	static GLuint CreateBuffer(GLenum type) noexcept;
	static std::uint8_t CreateBuffer8(GLenum type) noexcept;

	static GLuint CreateVAO() noexcept;
	static std::uint8_t CreateVAO8() noexcept;

	static void SetupUBO(std::uint8_t& ubo, GLuint index, std::size_t uboSize) noexcept;
	static void UpdateUBO(std::uint8_t& ubo, GLintptr offset, GLsizeiptr bytes, const void* data) noexcept;
};
// Image information
struct OGLImageInfo
{
	std::uint32_t width = 0, height = 0;
	std::vector<unsigned char> data;
};
// Hashing function for WorldPos vector struct
struct WorldPosHash
{
	std::size_t operator()(const WorldPos& vec) const noexcept;
};

// Mathematical functions
struct Math
{
	static float loopAround(float x, float min, float max) noexcept;
	static double loopAround(double x, double min, double max) noexcept;
	static constexpr int loopAroundInteger(int x, int minInc, int maxExcl) noexcept 
	{
		return minInc + ((maxExcl + x) % maxExcl);
	}

	static constexpr PosType qabs(PosType val) noexcept 
	{
		constexpr PosType mult = sizeof(PosType) * (CHAR_BIT - 1);
		const PosType mask = val >> mult;
		return (val + mask) ^ mask;
	}

	template<typename T>
	static constexpr T clamp(T val, T min, T max) noexcept
	{
		return val < min ? min : val > max ? max : val;
	}

	static constexpr double _sqrtInner(double val, double current, double previous) noexcept
	{
		return current == previous ? current : _sqrtInner(val, 0.5 * (current + val / current), current);
	}
	static constexpr double sqrt(double val) noexcept
	{
		return _sqrtInner(val, val, 0.0);
	}

	static constexpr int _ilogxInner(int i, int x, int c) noexcept
	{
		if (i == 1) return c;
		else return _ilogxInner(i / x, x, ++c);
	}
	static constexpr int ilogx(int i, int x) noexcept
	{
		return Math::_ilogxInner(i, x, 0);
	}

	static double lerpIndependent(double a, double b, double max) noexcept;

	static float lerp(float a, float b, float t) noexcept;
	static double lerp(double a, double b, double t) noexcept;

	static WorldPos toWorld(const glm::dvec3& v) noexcept;
	static PosType toWorld(double a) noexcept;

	static constexpr double PI_DBL = 3.141592653589793;
	static constexpr float PI_FLT = static_cast<float>(PI_DBL);

	static constexpr float PI_M2 = PI_FLT * 2.0f;
	static constexpr float HALF_PI = PI_FLT * 0.5f;
	static constexpr float RECIP_PI = 1.0f / PI_FLT;

	static constexpr double TODEGREES_DBL = 57.29577951308233;
	static constexpr double TORADIANS_DBL = 0.0174532925199433;
	static constexpr float TODEGREES_FLT = static_cast<float>(TODEGREES_DBL);
	static constexpr float TORADIANS_FLT = static_cast<float>(TORADIANS_DBL);
};

// Logging and text formatting
struct TextFormat
{
	static void warn(std::string t, std::string ttl) noexcept;
	static void log(std::string t, bool nl = true) noexcept;

	static bool stringEndsWith(const std::string& str, const std::string& ending) noexcept;
	static std::string getParentDirectory(const std::string& dir) noexcept;
};

// Vertex and fragment shader loader as well and disk reading
class Shader
{
public:
	void InitShader();
	enum class ShaderID { Blocks, Clouds, Inventory, Outline, Sky, Stars, Text, MAX };

	GLuint ShaderFromID(ShaderID id) const noexcept;
	void UseShader(ShaderID id) const noexcept;

	void SetBool(ShaderID id, const char* name, bool value) const noexcept;
	void SetInt(ShaderID id, const char* name, int value) const noexcept;
	void SetFloat(ShaderID id, const char* name, float value) const noexcept;

	static GLint GetLocation(GLuint shader, const char* name) noexcept;

	static void SetInt(GLint location, int value) noexcept;
	static void SetFloat(GLint location, float value) noexcept;

	static std::string ReadFileFromDisk(const std::string& filename);
	static void LoadTexture(OGLImageInfo& info, const char* filename, bool border, int filterParam, bool mipmap);

	~Shader() noexcept;
private:
	std::uint8_t m_programs[static_cast<int>(ShaderID::MAX)] {};
};

// Combined perlin noise struct
struct WorldNoise
{
	WorldNoise() noexcept = default;
	WorldNoise(WorldPerlin::NoiseSpline *splines);
	WorldNoise(WorldPerlin::NoiseSpline *splines, std::int64_t *seeds);
	
	WorldPerlin continentalness;
	WorldPerlin flatness;
	WorldPerlin depth;
	WorldPerlin temperature;
	WorldPerlin humidity;

	static constexpr int numNoises = 5;
};

// Globals
struct GameGlobalsObject
{
	GLFWwindow* window = nullptr;

	std::vector<std::thread> standaloneThreads;
	WorldNoise noiseGenerators;

	int windowX = 0, windowY = 0;
	int width = 0, height = 0;
	int updateInterval = 1;
	float aspect = 1.0f;

	float testfloat = 0.0f, testfloat2 = 0.0f;
	bool testbool = false;

	Shader shader;

	bool anyKeyPressed = false;
	bool mainLoopActive = false;
	bool focusChanged = true;
	bool chatting = false;
	bool minimized = false;
	bool isServer = false;
	bool noGeneration = false;
	bool showGUI = true;
	bool maxFPS = false;
	bool wireframe = false;

	double tickSpeed = 1.0;
	double tickedDeltaTime = 0.016;
	double gameTime = 0.0;

	double mouseX = 0.0, mouseY = 0.0;
	double deltaTime = 0.016;

	//std::string worldName;
	std::string currentDirectory;
	std::string resourcesFolder;

	OGLImageInfo blocksTextureInfo;
	OGLImageInfo textTextureInfo;
	OGLImageInfo inventoryTextureInfo;

	std::unordered_map<int, int> keyboardState;
} extern game;

#endif
