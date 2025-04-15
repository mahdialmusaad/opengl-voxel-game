#pragma once
#ifndef _SOURCE_UTILITY_UTILS_HEADER_
#define _SOURCE_UTILITY_UTILS_HEADER_

#define BADCRAFT_1THREAD

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_XYZW_ONLY
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <lodepng/lodepng.h>
#include <World/Generation/Perlin.hpp>

#include <concurrent_unordered_map.h>
#include <ppl.h>

#include <mutex>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <unordered_map>

namespace cnc = concurrency;

template <class T, class U>
constexpr T narrow_cast(U&& u) noexcept
{
	return static_cast<T>(std::forward<U>(u));
}
constexpr bool b_not(bool& x) noexcept
{
	x = !x;
	return x;
}

typedef int64_t PosType; // Switch between 32-bit and 64-bit block positions
typedef glm::vec<3, PosType> WorldPos;
typedef glm::vec<2, PosType> ChunkOffset;
typedef glm::vec<2, int8_t> RelativeOffset;

static constexpr PosType zeroPosType = static_cast<PosType>(0);
static constexpr PosType onePosType = static_cast<PosType>(1);
static constexpr size_t oneSizeT = static_cast<size_t>(1);

struct OGL 
{
	static GLuint CreateBuffer(GLenum type) noexcept;
	static uint8_t CreateBuffer8(GLenum type) noexcept;
	static uint16_t CreateBuffer16(GLenum type) noexcept;

	static GLuint CreateVAO() noexcept;
	static uint8_t CreateVAO8() noexcept;
	static uint16_t CreateVAO16() noexcept;

	static void SetupUBO(uint8_t& ubo, GLuint index, size_t uboSize) noexcept;
	static void UpdateUBO(uint8_t& ubo, GLintptr offset, GLsizeiptr bytes, const void* data) noexcept;
};
struct OGLImageInfo
{
	uint32_t width = 0, height = 0;
	std::vector<unsigned char> data;
};

struct WorldPosHash
{
	size_t operator()(const WorldPos& vec) const noexcept;
};

struct Math
{
	static float loopAround(float x, float min, float max) noexcept;
	static double loopAround(double x, double min, double max) noexcept;
	static int loopAroundInteger(int x, int minInc, int maxExcl) noexcept;

	static constexpr PosType abs(PosType val) noexcept 
	{
		constexpr PosType mult = sizeof(PosType) * static_cast<size_t>(CHAR_BIT - 1);
		const PosType mask = val >> mult;
		return (val + mask) ^ mask;
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

	static WorldPos toWorld(glm::dvec3 v) noexcept;
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

struct TextFormat
{
	static void warn(std::string t, std::string ttl);
	static void warnNull(std::string t);
	static void log(std::string t);
};

template <class T, class U>
constexpr T* malloc_T(U numInstances) noexcept
{
	void* res = malloc(static_cast<size_t>(numInstances) * sizeof(T));
	if (res == nullptr) { TextFormat::warnNull("Malloc_t result"); return malloc_T<T>(numInstances); }
	else return static_cast<T*>(res);
}

class Shader
{
public:
	void InitShader();
	Shader() noexcept = default;

	enum class ShaderID { Cloud, Inventory, Outline, Sky, Stars, Text, World, MAX };

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
private:
	uint8_t m_programs[static_cast<int>(ShaderID::MAX)]{};
};

struct WorldNoise
{
	WorldNoise() noexcept = default;
	WorldNoise(WorldPerlin::NoiseSpline *splines);
	WorldNoise(WorldPerlin::NoiseSpline *splines, uint64_t *seeds);
	
	WorldPerlin continentalness;
	WorldPerlin flatness;
	WorldPerlin depth;
	WorldPerlin temperature;
	WorldPerlin humidity;
	static constexpr int numNoises = 5;
};

struct Game
{
	GLFWwindow* window = nullptr;

	std::vector<std::thread> standaloneThreads;
	WorldNoise noiseGenerators;

	int windowX = 0, windowY = 0;
	int width = 0, height = 0;
	int updateInterval = 1;
	float aspect = 1.5f;

	Shader shader;

	bool anyKeyPressed = false;
	bool mainLoopActive = false;
	bool focusChanged = true;
	bool chatting = false;
	bool minimized = false;
	bool isServer = false;
	bool test = false;

	double tickSpeed = 1.0;
	double tickedDeltaTime = 0.016;

	double currentFrameTime = 0.0;
	double tickedFrameTime = 0.0;

	double mouseX = 0.0, mouseY = 0.0;
	double deltaTime = 0.016;

	std::string worldName;

	OGLImageInfo blocksTextureInfo;
	OGLImageInfo textTextureInfo;
	OGLImageInfo inventoryTextureInfo;

	std::unordered_map<int, int> keyboardState;
} inline game;

#endif
