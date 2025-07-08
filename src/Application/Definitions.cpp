#include "Definitions.hpp"

// -------------------- Math struct -------------------- 

std::size_t Math::WPHash::operator()(const WorldPos &vec) const noexcept
{
	return 
		static_cast<std::size_t>(vec.x) ^ 
		(
			((static_cast<std::size_t>(vec.y) << static_cast<std::size_t>(1u)) ^ 
			(static_cast<std::size_t>(vec.z) << static_cast<std::size_t>(1u))
		) >> static_cast<std::size_t>(1u));
}
int Math::loopAround(int x, int minInc, int maxExcl) noexcept { return minInc + ((maxExcl + x) % maxExcl); }
bool Math::isLargeOffset(const WorldPos &pos, PosType threshold) noexcept { return glm::max(glm::max(glm::abs(pos.x), glm::abs(pos.y)), glm::abs(pos.z)) >= threshold; }

// -------------------- TextFormat struct -------------------- 

void TextFormat::log(std::string t, bool nl) noexcept { fmt::print("[ {:.3f}ms ] {}{}",  glfwGetTime() * 1000.0, t, (nl ? "\n" : "")); }
void TextFormat::warn(std::string t, std::string ttl) noexcept
{
	std::cout << "\n";
	static std::string stars = std::string(25, '*');
	log(fmt::format("{} {}", stars, ttl), false);
	fmt::print(" {}\n{}\n", stars, t);
}

bool TextFormat::stringEndsWith(const std::string &str, const std::string &ending) noexcept
{
	return str.compare(str.length() - ending.length(), ending.length(), ending) == 0;
}
std::string TextFormat::getParentDirectory(const std::string &dir) noexcept
{
	const std::size_t pos = dir.find_last_of("\\/");
	return pos == std::string::npos ? "" : dir.substr(0u, pos);
}

std::int64_t TextFormat::strtoi64(const std::string &numText) noexcept
{
	int64_t val;
	std::istringstream valStr(numText);
	valStr >> val;
	return val;
}

// -------------------- OGL struct -------------------- 

GLuint OGL::CreateVAO(bool bind) noexcept
{
	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	if (bind) glBindVertexArray(VAO);
	return VAO;
}
GLuint OGL::CreateBuffer(GLenum type) noexcept
{
	GLuint buf;
	glGenBuffers(1, &buf);
	glBindBuffer(type, buf);
	return buf;
}
void OGL::SetupUBO(GLuint &ubo, GLuint index, std::size_t uboSize) noexcept
{
	ubo = OGL::CreateBuffer(GL_UNIFORM_BUFFER);
	glBufferStorage(GL_UNIFORM_BUFFER, uboSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferBase(GL_UNIFORM_BUFFER, index, ubo);
}
void OGL::UpdateUBO(GLuint &ubo, GLintptr offset, GLsizeiptr bytes, const void *data) noexcept
{
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, offset, bytes, data);
}

std::string OGL::GetString(GLenum stringID) noexcept
{
	return std::string(reinterpret_cast<const char*>(glGetString(stringID)));
}

// -------------------- Shader struct -------------------- 

void Shader::InitShader()
{
	static const auto CheckProgramError = [&](GLuint program, const std::string &vertex, const std::string &fragment)
	{
		// Check for any errors in the given program ID
		int success;
		glGetProgramiv(program, GL_LINK_STATUS, &success);

		if (!success) {
			int length = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
			char *infolog = new char[length] {};
			glGetProgramInfoLog(program, length, nullptr, infolog);

			TextFormat::warn(
				fmt::format("Linked vertex shader: {}\nLinked fragment shader: {}\nError message:\n{}", vertex, fragment, infolog), 
				"Shader Program Error - ID: " + fmt::to_string(program)
			);

			delete[] infolog;
		}
	};
	static const auto CheckShaderError = [&](const std::string &filename, GLuint shader)
	{
		// Check for any errors in the given vertex or fragment shader ID
		int success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

		if (!success) {
			int length = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
			char *infolog = new char[length] {};
			glGetShaderInfoLog(shader, length, nullptr, infolog);

			TextFormat::warn(
				fmt::format("Filename: {}\nError message:\n{}", filename, infolog), 
				"Shader Error - ID: " + fmt::to_string(shader)
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
		const char *c_contents = contents.c_str();

		// Create shader with string and compile and check for any errors in the GLSL contents
		glShaderSource(shaderID, 1, &c_contents, nullptr);
		glCompileShader(shaderID);
		const bool err = CheckShaderError(filename, shaderID);

		// Return shader identifier
		return err ? static_cast<GLuint>(0u) : shaderID;
	};

	// Load all of the vertex and fragment shaders and then create a shader program with them
	const int numPrograms = static_cast<int>(ShaderID::MAX);
	
	// Load order should match the ShaderID enum
	typedef std::pair<std::string, std::string> FilePair;
	static const FilePair shaderPairs[numPrograms] = {
		{"Blocks.vert", "Blocks.frag"},
		{"Clouds.vert", "Clouds.frag"},
		{"Inventory.vert", "Inventory.frag"},
		{"Outline.vert", "Outline.frag"},
		{"Sky.vert", "Sky.frag"},
		{"Stars.vert", "Stars.frag"},
		{"Text.vert", "Text.frag"},
		{"Planets.vert", "Planets.frag"},
		{"Vec3.vert", "Vec3.frag"},
		{"Border.vert", "Border.frag"}
	};

	const std::string shadersFile = "Shaders\\";

	// Load each vertex and fragment pair from disk and create a shader program from them
	for (int programIndex = 0; programIndex < numPrograms; ++programIndex) {
		const FilePair &pair = shaderPairs[programIndex];
		const std::string vertexFileName = pair.first, fragmentFileName = pair.second;

		// Create shaders from the associated files
		const GLuint vertexID = CreateShaderFromFilename(game.resourcesFolder + shadersFile + vertexFileName),
			fragmentID = CreateShaderFromFilename(game.resourcesFolder + shadersFile + fragmentFileName);

		// Stop and throw error if any program failed to compile
		if (!vertexID || !fragmentID) { 
			throw std::runtime_error("Program compilation failed"); 
			return; 
		}

		// Create new shader using the vertex and fragment shaders (delete original if exists)
		GLuint &currentProgram = m_programs[programIndex];
		if (currentProgram) glDeleteProgram(currentProgram);
		currentProgram = glCreateProgram(); 

		glAttachShader(currentProgram, vertexID);
		glAttachShader(currentProgram, fragmentID);
		glLinkProgram(currentProgram);

		// Delete shaders after link success to free up memory as they are no longer needed
		glDeleteShader(vertexID);
		glDeleteShader(fragmentID);

		// Check for any shader program errors
		CheckProgramError(currentProgram, vertexFileName, fragmentFileName);
		
		TextFormat::log(
			fmt::format("Shader program {} '{}' created using {} ({}) and {} ({})", 
			currentProgram, vertexFileName.substr(0, vertexFileName.size()-5), vertexFileName, vertexID, fragmentFileName, fragmentID
		));
	}
}

GLuint Shader::IDFromShaderName(ShaderID id) const noexcept
{
	return m_programs[static_cast<int>(id)];
}
void Shader::UseShader(ShaderID id) const noexcept
{
	glUseProgram(IDFromShaderName(id));
}

GLint Shader::GetLocation(ShaderID id, const char *name) noexcept
{ 
	return glGetUniformLocation(IDFromShaderName(id), name);
}

std::string Shader::ReadFileFromDisk(const std::string &filename)
{
	// Check if file exists
	if (!FileManager::FileExists(filename)) {
		TextFormat::warn(fmt::format("File {} does not exist!", filename), "Invalid filename");
		return "";
	}

	// Setup file stream
	std::ifstream fileStream(filename, std::ios::binary | std::ios::ate);

	// Check if stream is valid
	if (fileStream.good()) {
		const std::streampos fileSize = fileStream.tellg(); // Get file size
		fileStream.seekg(std::ios::beg); // Move reader to beginning
		std::string fileContents(fileSize, 0); // Create string with allocated size
		fileStream.read(&fileContents[0], fileSize); // Read file, placing data in created string

		// Return string with file contents
		if (game.mainLoopActive) TextFormat::log(fmt::format("File '{}' read from disk.", filename));
		return fileContents;
	}

	TextFormat::warn(fmt::format("File stream failed to initialize on '{}'", filename), "File read error");
	return "";
}

void Shader::LoadImage(OGLImageInfo &info, const char *filename, bool border, int filterParam, bool mipmap)
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
	
	const GLint texWrapParam = border ? GL_CLAMP_TO_BORDER : GL_REPEAT;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texWrapParam);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texWrapParam);

	if (border) {
		const float borderColours[4] {}; // RGBA 0 - transparent colour outside texture bounds
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColours);
	}

	// Load in found image whilst setting data
	if (lodepng::decode(info.data, info.width, info.height, (game.resourcesFolder + filename).c_str())) throw std::runtime_error("Image load fail");

	// Save the texture data into the currently bound texture object
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, info.width, info.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, info.data.data());

	// Create mipmaps for the texture
	if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);

	TextFormat::log("Loaded image: " + std::string(filename));
}

Shader::~Shader() noexcept
{
	// Delete all valid shader programs
	for (const GLuint &program : m_programs) {
		if (program == 0u) continue;
		glDeleteProgram(program);
	}
}

// -------------------- WorldNoise struct -------------------- 

WorldNoise::WorldNoise(WorldPerlin::NoiseSpline *splines, std::int64_t *seeds)
{
	WorldPerlin *perlins[] = { &continentalness, &flatness, &depth, &temperature, &humidity };
	for (int i = 0; i < NoiseEnums::MAX; ++i) {
		WorldPerlin *perlin = perlins[i];
		perlin->noiseSplines = splines[i];
		perlin->ChangeSeed(seeds[i]);
	}
}
WorldNoise::WorldNoise(WorldPerlin::NoiseSpline *splines)
{
	WorldPerlin *perlins[] = { &continentalness, &flatness, &depth, &temperature, &humidity };
	bool hasSplines = splines != nullptr;

	for (int i = 0; i < NoiseEnums::MAX; ++i) {
		WorldPerlin *perlin = perlins[i];
		if (hasSplines) perlin->noiseSplines = splines[i];
		perlin->ChangeSeed(); 
	}
}

// -------------------- FileManager struct -------------------- 

bool FileManager::DirectoryExists(std::string path)
{
	struct stat info;
	return stat(path.c_str(), &info) == 0 && info.st_mode & S_IFDIR;
}
bool FileManager::FileExists(std::string path) 
{
	return std::ifstream(path).good();
}
bool FileManager::Exists(std::string path)
{
	return DirectoryExists(path) || FileExists(path);
}
void FileManager::DeletePath(std::string path)
{
	if (std::remove(path.c_str()) != 0) throw FileError(fmt::format("Failed to delete path '{}'", path));
}
void FileManager::CreatePath(std::string path)
{
	if (mkdir(path.c_str()) != 0) throw FileError(fmt::format("Failed to create path '{}'", path));
}
