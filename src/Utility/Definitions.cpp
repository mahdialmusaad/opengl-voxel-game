#include <Utility/Definitions.hpp>

size_t WorldPosHash::operator()(const WorldPos& vec) const noexcept
{
	return 
		static_cast<size_t>(vec.x) ^ 
		(
			((static_cast<size_t>(vec.y) << onePosType) ^ 
			(static_cast<size_t>(vec.z) << onePosType)
		) >> onePosType);
}
double Math::loopAround(double x, double min, double max) noexcept
{
	return (x < min) ? (max - (min - x)) : ((x > max) ? (min + (x - max)) : x);
}
float Math::loopAround(float x, float min, float max) noexcept
{
	return (x < min) ? (max - (min - x)) : ((x > max) ? (min + (x - max)) : x);
}
int Math::loopAroundInteger(int x, int minInc, int maxExcl) noexcept
{
	return minInc + ((maxExcl + x) % maxExcl);
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


void TextFormat::log(std::string t)
{
	std::cout << "[ " << glfwGetTime() * 1000.0f << "ms ]\t" << t << "\n";
}
void TextFormat::warn(std::string t, std::string ttl)
{
	std::cout << "\n---------------- " << ttl << " ----------------\n";
	log(t);
	std::cout << "-----------------------------------------\n";
}
void TextFormat::warnNull(std::string t)
{
	std::cout << "\n------\n";
	log("Malloc returned null on: ");
	std::cout << "\n------\n";
}
GLuint OGL::CreateVAO() noexcept
{
	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	return VAO;
}
uint8_t OGL::CreateVAO8() noexcept
{
	return narrow_cast<uint8_t>(CreateVAO());
}
uint16_t OGL::CreateVAO16() noexcept
{
	return narrow_cast<uint16_t>(CreateVAO());
}

GLuint OGL::CreateBuffer(GLenum type) noexcept
{
	GLuint buf;
	glGenBuffers(1, &buf);
	glBindBuffer(type, buf);
	return buf;
}
uint8_t OGL::CreateBuffer8(GLenum type) noexcept
{
	return narrow_cast<uint8_t>(CreateBuffer(type));
}
uint16_t OGL::CreateBuffer16(GLenum type) noexcept
{
	return narrow_cast<uint16_t>(CreateBuffer(type));
}

void OGL::SetupUBO(uint8_t& ubo, GLuint index, size_t uboSize) noexcept
{
	ubo = OGL::CreateBuffer8(GL_UNIFORM_BUFFER);
	glBufferStorage(GL_UNIFORM_BUFFER, uboSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferBase(GL_UNIFORM_BUFFER, index, ubo);
}
void OGL::UpdateUBO(uint8_t& ubo, GLintptr offset, GLsizeiptr bytes, const void* data) noexcept
{
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, offset, bytes, data);
}

void Shader::InitShader()
{
	static constexpr auto CheckProgramError = [&](GLuint program, const std::string& vertex, const std::string& fragment) noexcept
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
				std::format("Linked vertex shader: {}\nLinked fragment shader: {}\nError message:\n{}", vertex, fragment, infolog),
				std::format("Program (ID: {}) Error", program)
			);

			delete[] infolog;
		}
	};

	static constexpr auto CheckShaderError = [&](const std::string& filename, GLuint shader) noexcept
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
				std::format("Filename: {}\nError message:\n{}", filename, infolog),
				std::format("Shader (ID: {}) Error", shader)
			);

			delete[] infolog;
		}
	};

	static constexpr auto CreateShaderFromFilename = [&](const std::string& filename) noexcept
	{
		// Determine if the current file is a vertex file or fragment file
		const GLuint shaderID = glCreateShader(filename.ends_with(".vert") ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);

		// Read file contents into std::string and convert to C-style string for OGL shader creation
		const std::string contents = ReadFileFromDisk(filename);
		const char* c_contents = contents.c_str();

		// Create shader with string and compile and check for any errors in the GLSL contents
		glShaderSource(shaderID, 1, &c_contents, nullptr);
		glCompileShader(shaderID);
		CheckShaderError(filename, shaderID);

		// Return shader identifier
		return shaderID;
	};

	// Clear any existing shaders when reloading
	if (m_programs[0] != 0) {
		// Delete program using ID and set array value 0 (can easily be identified as deleted since 0 is not a valid ID)
		for (uint8_t& program : m_programs) { glDeleteProgram(static_cast<GLuint>(program)); program = 0; }
	}

	// Load all of the vertex and fragment shaders and then create a shader program with them
	constexpr int numPrograms = static_cast<int>(ShaderID::MAX);
	const std::string shaderDir = "src/Shaders/";

	const std::pair<std::string, std::string> shaderPairs[numPrograms] = {
		{"Cld.vert", "Cld.frag"},
		{"Inv.vert", "Inv.frag"},
		{"Out.vert", "Out.frag"},
		{"Sky.vert", "Sky.frag"},
		{"Str.vert", "Str.frag"},
		{"Txt.vert", "Txt.frag"},
		{"Wld.vert", "Wld.frag"},
	};

	for (int programIndex = 0; programIndex < numPrograms; ++programIndex) {
		const auto& [vertexFileName, fragmentFileName] = shaderPairs[programIndex];

		// Create shaders from the associated files
		const GLuint vertexID = CreateShaderFromFilename(shaderDir + vertexFileName),
					 fragmentID = CreateShaderFromFilename(shaderDir + fragmentFileName);

		// Create new shader using the vertex and fragment shaders
		const GLuint programID = glCreateProgram();
		glAttachShader(programID, vertexID);
		glAttachShader(programID, fragmentID);
		glLinkProgram(programID);

		// Delete shaders after link success
		glDeleteShader(vertexID);
		glDeleteShader(fragmentID);

		// Check for any shader program errors
		CheckProgramError(programID, vertexFileName, fragmentFileName);

		// Add to shader program array
		m_programs[programIndex] = narrow_cast<uint8_t>(programID);

		TextFormat::log(
			std::format(
				"Shader program created ({}), Vertex ({}): {}, Fragment ({}): {}", 
				programID, vertexID, vertexFileName, fragmentID, fragmentFileName
			)
		);
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

void Shader::SetFloat(ShaderID id, const char* name, float value) const noexcept
{
	const GLuint program = ShaderFromID(id);
	glUseProgram(program);
	SetFloat(GetLocation(program, name), value);
}

GLint Shader::GetLocation(GLuint shader, const char* name) noexcept
{
	return glGetUniformLocation(shader, name);
}

void Shader::SetInt(GLint location, int value) noexcept
{
	glUniform1i(location, value);
}

void Shader::SetFloat(GLint location, float value) noexcept
{
	glUniform1f(location, value);
}

std::string Shader::ReadFileFromDisk(const std::string& filename)
{
	typedef const std::istreambuf_iterator<char> charFileStream;
	constexpr const charFileStream cfs;

	std::fstream fileStream = std::fstream(filename);
	const std::string contents = std::string(charFileStream(fileStream), cfs);

	TextFormat::log("\'" + filename + "\' read from disk");
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
	if (lodepng::decode(info.data, info.width, info.height, filename)) throw std::runtime_error("Image fail");

	// Save the texture data into the currently bound texture object
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, info.width, info.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, info.data.data());

	// Create mipmaps for the texture
	if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);

	TextFormat::log("Texture loaded, name: " + std::string(filename));
}


WorldNoise::WorldNoise(WorldPerlin::NoiseSpline *splines, uint64_t *seeds)
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
	for (int i = 0; i < numNoises; ++i) {
		WorldPerlin *perlin = perlins[i];
		perlin->noiseSplines = splines[i];
		perlin->ChangeSeed(1337ui64); 
	}
}
