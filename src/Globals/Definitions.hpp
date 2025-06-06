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
	static GLuint CreateVAO() noexcept;

	static void SetupUBO(GLuint& ubo, GLuint index, std::size_t uboSize) noexcept;
	static void UpdateUBO(GLuint& ubo, GLintptr offset, GLsizeiptr bytes, const void* data) noexcept;
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
	static int loopAround(int x, int minInc, int maxExcl) noexcept;

	static PosType absInt(PosType val) noexcept;

	static float lerp(float a, float b, float t) noexcept;
	static double lerp(double a, double b, double t) noexcept;

	static WorldPos toWorld(const glm::dvec3& v) noexcept;
	static PosType toWorld(double a) noexcept;
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
	enum class ShaderID { Blocks, Clouds, Inventory, Outline, Sky, Stars, Text, SunMoon, MAX };

	static GLint GetLocation(GLuint shader, const char* name) noexcept;
	GLuint ShaderFromID(ShaderID id) const noexcept;
	void UseShader(ShaderID id) const noexcept;

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

	glm::vec4 testvals { 1.0f, 1.0f, 1.0f, 1.0f };
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
