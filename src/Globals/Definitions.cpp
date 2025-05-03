#include "Definitions.hpp"

// -------------------- Math struct -------------------- 

std::size_t WorldPosHash::operator()(const WorldPos& vec) const noexcept
{
	return 
		static_cast<std::size_t>(vec.x) ^ 
		(
			((static_cast<std::size_t>(vec.y) << static_cast<std::size_t>(1)) ^ 
			(static_cast<std::size_t>(vec.z) << static_cast<std::size_t>(1))
		) >> static_cast<std::size_t>(1));
}
double Math::loopAround(double x, double min, double max) noexcept
{
	return (x < min) ? (max - (min - x)) : ((x > max) ? (min + (x - max)) : x);
}
float Math::loopAround(float x, float min, float max) noexcept
{
	return (x < min) ? (max - (min - x)) : ((x > max) ? (min + (x - max)) : x);
}
double Math::lerpIndependent(double a, double b, double max) noexcept
{
	double res = b - a;
	if (fabs(res) <= max) return b;
	return a + (res >= 0.0 ? 1.0 : -1.0) * max;
}
float Math::lerp(float a, float b, float t) noexcept
{
	return a + (b - a) * t;
}
double Math::lerp(double a, double b, double t) noexcept
{
	return a + (b - a) * t;
}
WorldPos Math::toWorld(glm::dvec3 v) noexcept
{
	return { toWorld(v.x), toWorld(v.y), toWorld(v.z) };
}
PosType Math::toWorld(double a) noexcept
{
	const PosType pos = static_cast<PosType>(a);
	return a < 0.0 ? pos - 1 : pos;
}

// -------------------- TextFormat struct -------------------- 

void TextFormat::log(std::string t, bool nl) noexcept
{
	// Print out string with current time for logging purposes
	fmt::print("[ {:.3f}ms ]\t{}{}", glfwGetTime() * 1000.0, t, (nl ? "\n" : ""));
}
void TextFormat::warn(std::string t, std::string ttl) noexcept
{
	// Warning log for any issues
	std::cout << "\n";
	log("************************ " + ttl, false);
	std::cout << " ************************\n" << t << "\n"; 
}

bool TextFormat::stringEndsWith(const std::string &str, const std::string &ending) noexcept
{
	// std::string::ends_with only available in c++20, trying not to rely on new versions especially since 
	// there is no guarantee that very new functions may not be supported on all compilers/platforms.
	return str.compare(str.length() - ending.length(), ending.length(), ending) == 0;
}
std::string TextFormat::getParentDirectory(const std::string& dir) noexcept
{
	const std::size_t pos = dir.find_last_of("\\/");
	return pos == std::string::npos ? "" : dir.substr(0u, pos);
}

// -------------------- OGL struct -------------------- 

GLuint OGL::CreateVAO() noexcept
{
	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	return VAO;
}
std::uint8_t OGL::CreateVAO8() noexcept
{
	return static_cast<std::uint8_t>(CreateVAO());
}
GLuint OGL::CreateBuffer(GLenum type) noexcept
{
	GLuint buf;
	glGenBuffers(1, &buf);
	glBindBuffer(type, buf);
	return buf;
}
std::uint8_t OGL::CreateBuffer8(GLenum type) noexcept
{
	return static_cast<std::uint8_t>(CreateBuffer(type));
}
void OGL::SetupUBO(std::uint8_t& ubo, GLuint index, std::size_t uboSize) noexcept
{
	ubo = OGL::CreateBuffer8(GL_UNIFORM_BUFFER);
	glBufferStorage(GL_UNIFORM_BUFFER, uboSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferBase(GL_UNIFORM_BUFFER, index, ubo);
}
void OGL::UpdateUBO(std::uint8_t& ubo, GLintptr offset, GLsizeiptr bytes, const void* data) noexcept
{
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, offset, bytes, data);
}

// -------------------- Shader struct -------------------- 

void Shader::InitShader()
{
	static const auto CheckProgramError = [&](GLuint program, const std::string& vertex, const std::string& fragment)
	{
		// Check for any errors in the given program ID
		int success;
		glGetProgramiv(program, GL_LINK_STATUS, &success);

		if (!success) {
			int length = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
			char* infolog = new char[length] {};
			glGetProgramInfoLog(program, length, nullptr, infolog);

			TextFormat::warn(
				fmt::format("Linked vertex shader: {}\nLinked fragment shader: {}\nError message:\n{}", vertex, fragment, infolog), 
				"Shader Program Error - ID: " + std::to_string(program)
			);

			delete[] infolog;
		}
	};
	static const auto CheckShaderError = [&](const std::string& filename, GLuint shader)
	{
		// Check for any errors in the given vertex or fragment shader ID
		int success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

		if (!success) {
			int length = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
			char* infolog = new char[length] {};
			glGetShaderInfoLog(shader, length, nullptr, infolog);

			TextFormat::warn(
				fmt::format("Filename: {}\nError message:\n{}", filename, infolog), 
				"Shader Error - ID: " + std::to_string(shader)
			);

			delete[] infolog;
			return true;
		}

		return false;
	};
	static const auto CreateShaderFromFilename = [&](const std::string filename)
	{
		// Determine if the current file is a vertex file or fragment file
		const std::string vertend = ".vert";
		const GLuint shaderID = glCreateShader(TextFormat::stringEndsWith(filename, vertend) ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);

		// Read file contents into std::string and convert to C-style string for OGL shader creation
		const std::string contents = ReadFileFromDisk(filename);
		const char* c_contents = contents.c_str();

		// Create shader with string and compile and check for any errors in the GLSL contents
		glShaderSource(shaderID, 1, &c_contents, nullptr);
		glCompileShader(shaderID);
		const bool err = CheckShaderError(filename, shaderID);

		// Return shader identifier
		return err ? static_cast<GLuint>(0u) : shaderID;
	};

	// Clear any existing shaders when reloading
	if (m_programs[0] != 0) {
		// Set program to none
		glUseProgram(0u);
		// Delete program using ID and set array value 0 (can easily be identified as deleted since 0 is not a valid ID)
		for (std::uint8_t& program : m_programs) { 
			glDeleteProgram(static_cast<GLuint>(program));
			program = 0;
		}
	}

	// Load all of the vertex and fragment shaders and then create a shader program with them
	constexpr int numPrograms = static_cast<int>(ShaderID::MAX);
	
	// Load order should match the ShaderID enum
	const std::pair<std::string, std::string> shaderPairs[numPrograms] = {
		{"Blocks.vert", "Blocks.frag"},
		{"Clouds.vert", "Clouds.frag"},
		{"Inventory.vert", "Inventory.frag"},
		{"Outline.vert", "Outline.frag"},
		{"Sky.vert", "Sky.frag"},
		{"Stars.vert", "Stars.frag"},
		{"Text.vert", "Text.frag"},
	};

	const std::string shadersFile = "Shaders\\";

	// Load each vertex and fragment pair from disk and create a shader program from them
	for (int programIndex = 0; programIndex < numPrograms; ++programIndex) {
		const auto& [vertexFileName, fragmentFileName] = shaderPairs[programIndex];

		// Create shaders from the associated files
		const GLuint vertexID = CreateShaderFromFilename(game.resourcesFolder + shadersFile + vertexFileName),
			fragmentID = CreateShaderFromFilename(game.resourcesFolder + shadersFile + fragmentFileName);

		// Stop and throw error if any program failed to compile
		if (!vertexID || !fragmentID) { 
			throw std::runtime_error("Program compilation failed"); 
			return; 
		}

		// Create new shader using the vertex and fragment shaders
		const GLuint programID = glCreateProgram();
		glAttachShader(programID, vertexID);
		glAttachShader(programID, fragmentID);
		glLinkProgram(programID);

		// Detach and delete shaders after link success to free up memory as they are no longer needed
		glDetachShader(vertexID, programID);
		glDeleteShader(vertexID);
		
		glDetachShader(fragmentID, programID);
		glDeleteShader(fragmentID);

		// Check for any shader program errors
		CheckProgramError(programID, vertexFileName, fragmentFileName);

		// Add to shader program array
		m_programs[programIndex] = static_cast<std::uint8_t>(programID);
		
		TextFormat::log(fmt::format("Shader program {} created using {} and {}", programID, vertexFileName, fragmentFileName));
	}
}

GLuint Shader::ShaderFromID(ShaderID id) const noexcept
{
	return static_cast<GLuint>(m_programs[static_cast<int>(id)]);
}
void Shader::UseShader(ShaderID id) const noexcept
{
	glUseProgram(ShaderFromID(id));
}

void Shader::SetBool(ShaderID id, const char* name, bool value) const noexcept
{
	const GLuint program = ShaderFromID(id);
	glUseProgram(program);
	SetInt(GetLocation(program, name), value ? 0 : 1);
}

void Shader::SetInt(ShaderID id, const char* name, int value) const noexcept
{
	const GLuint program = ShaderFromID(id);
	glUseProgram(program);
	SetInt(GetLocation(program, name), value);
}
void Shader::SetInt(GLint location, int value) noexcept
{
	glUniform1i(location, value);
}

void Shader::SetFloat(ShaderID id, const char* name, float value) const noexcept
{
	const GLuint program = ShaderFromID(id);
	glUseProgram(program);
	SetFloat(GetLocation(program, name), value);
}
void Shader::SetFloat(GLint location, float value) noexcept
{
	glUniform1f(location, value);
}

GLint Shader::GetLocation(GLuint shader, const char* name) noexcept
{
	return glGetUniformLocation(shader, name);
}

std::string Shader::ReadFileFromDisk(const std::string& filename)
{
	typedef const std::istreambuf_iterator<char> charFileStream;
	constexpr const charFileStream cfs;

	if (!std::filesystem::exists(filename)) {
		TextFormat::log("WARNING: File " + filename + " does not exist!");
		return std::string();
	}

	std::fstream fileStream = std::fstream(filename);
	const std::string contents = std::string(charFileStream(fileStream), cfs);

	if (game.mainLoopActive) TextFormat::log("\'" + filename + "\' read from disk");
	return contents;
}

void Shader::LoadTexture(OGLImageInfo& info, const char* filename, bool border, int filterParam, bool mipmap)
{
	// Create a new texture object
	GLuint tex;
	glGenTextures(1, &tex);

	// Set the newly created texture as the currently bound one 
	glActiveTexture(GL_TEXTURE0 + (tex - 1)); // GL_TEXTURE0, GL_TEXTURE1, ...
	glBindTexture(GL_TEXTURE_2D, tex);

	// Use nearest texture filtering for blocky texture appearance and mipmap selection
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterParam);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterParam);

	// Texture coordinate overflow handling - show a specific colour, repeat the texture, etc
	constexpr float borderColours[4] {};
	const GLint texWrapParam = border ? GL_CLAMP_TO_BORDER : GL_REPEAT;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texWrapParam);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texWrapParam);

	if (border) glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColours);

	// Load in found image whilst setting data
	if (lodepng::decode(info.data, info.width, info.height, (game.resourcesFolder + filename).c_str())) throw std::runtime_error("Image load fail");

	// Save the texture data into the currently bound texture object
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, info.width, info.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, info.data.data());

	// Create mipmaps for the texture
	if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);

	TextFormat::log("Loaded texture: " + std::string(filename));
}

Shader::~Shader() noexcept
{
	// Delete all valid shader programs
	for (const uint8_t& program : m_programs) {
		if (program == 0) continue;
		glDeleteProgram(program);
	}
}

// -------------------- WorldNoise struct -------------------- 

WorldNoise::WorldNoise(WorldPerlin::NoiseSpline *splines, std::int64_t *seeds)
{
	WorldPerlin* perlins[] = { &continentalness, &flatness, &depth, &temperature, &humidity };
	for (int i = 0; i < numNoises; ++i) {
		WorldPerlin *perlin = perlins[i];
		perlin->noiseSplines = splines[i];
		perlin->ChangeSeed(seeds[i]);
	}
}
WorldNoise::WorldNoise(WorldPerlin::NoiseSpline *splines)
{
	WorldPerlin* perlins[] = { &continentalness, &flatness, &depth, &temperature, &humidity };
	bool DEBUGNOSPLINES = splines == nullptr;

	for (int i = 0; i < numNoises; ++i) {
		WorldPerlin *perlin = perlins[i];
		if (!DEBUGNOSPLINES) perlin->noiseSplines = splines[i];
		perlin->ChangeSeed(); 
	}
}
