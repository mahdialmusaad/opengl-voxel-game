#include "Definitions.hpp"

// (Math namespace is all inline for now)

// -------------------- TextFormat -------------------- 

void TextFormat::log(std::string t, bool nl) noexcept { fmt::print("[ {:.3f}ms ] {}{}",  glfwGetTime() * 1000.0, t, (nl ? "\n" : "")); }
void TextFormat::warn(std::string t, std::string ttl) noexcept
{
	fmt::print("\n");
	const std::string stars = std::string(25, '*');
	log(fmt::format("{} {}", stars, ttl), false);
	fmt::print(" {}\n{}\n", stars, t);
}

int TextFormat::numChar(const std::string &str, const char c) noexcept
{
	return static_cast<int>(std::count(str.begin(), str.end(), c));
}
bool TextFormat::stringEndsWith(const std::string &str, const std::string &ending) noexcept
{
	return str.compare(str.length() - ending.length(), ending.length(), ending) == 0;
}
void TextFormat::strsplit(std::vector<std::string> &vec, const std::string &str, const char sp) noexcept
{
	std::size_t lastIndex{};
	// Use 'do...while' as 'lastIndex' is initially 0 and only changed as part of the split logic - 
	// a 'while loop' will not be entered at all as 'lastIndex' is checked initially and evaluates to false.
	do {
		std::size_t splitIndex = str.find(sp, lastIndex); // Get next index of split character in string
		const std::string arg = str.substr(lastIndex, splitIndex - lastIndex); // Get substring of text between split character
		vec.emplace_back(arg); // Add text in between split character to given vector
		lastIndex = ++splitIndex; // Start of next substring is after split character so it doesn't get detected again
	} while (lastIndex);
	// If there is no space at the end, 'splitIndex' will become std::string::npos so the '.substr' uses the rest of the string.
	// Since 'lastIndex' is set to 1 more than 'splitIndex' (pre-inc), it overflows to 0 (size_t is unsigned) and evaluates to false.
	
}
std::size_t TextFormat::findNthOf(const std::string &str, const char find, int nth) noexcept
{
	if (nth < 1) return std::size_t{};
	std::size_t index = std::string::npos;
	do index = str.find(find, ++index); while (--nth && index != std::string::npos);
	return index;
}
std::intmax_t TextFormat::strtoimax(const std::string &numText)
{
	std::intmax_t val;
	std::istringstream valStr(numText);
	valStr >> val;
	if (valStr.fail()) throw std::invalid_argument("Invalid intmax_t conversion");
	return val;
}

// -------------------- OGL -------------------- 

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
void OGL::UpdateUBO(GLuint &ubo, const void *data, std::size_t bytes, std::size_t offset) noexcept
{
	game.perfs.uboUpdate.Start();
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(bytes), data);
	game.perfs.uboUpdate.End();
}

std::string OGL::GetString(GLenum stringID) noexcept { return reinterpret_cast<const char*>(glGetString(stringID)); }

// -------------------- Shader -------------------- 

void Shader::InitShaders() { EachProgram([&](Program &prog) { InitProgram(prog); }); }
void Shader::UseProgram(Program &program) const noexcept { glUseProgram(program.program); }

GLint Shader::GetLocation(Program &program, const char *name) const noexcept
{
	UseProgram(program);
	const GLchar* glCharName = reinterpret_cast<const GLchar*>(name);
	return glGetUniformLocation(program.program, glCharName);
}

GLuint Shader::CreateShader(const std::string &fileData, const std::string &name, bool isVertex) noexcept
{
	// Create either a vertex or fragment shader
	const GLuint shaderID = glCreateShader(isVertex ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
	
	// Create shader with GLSL code and compile
	const char* data = fileData.c_str();
	glShaderSource(shaderID, 1, &data, nullptr);
	glCompileShader(shaderID);

	// Check for any errors during shader code compilation
	return !CheckShaderStatus(shaderID, name) ? GLuint{} : shaderID; // Check for errors and return shader ID if not
}

bool Shader::CheckShaderStatus(GLuint shader, const std::string &name) noexcept
{
	// Check for any errors in the given vertex or fragment shader ID
	int value; // GL_FALSE = error, GL_TRUE = success
	glGetShaderiv(shader, GL_COMPILE_STATUS, &value);

	if (value == GL_TRUE) return true; // True - no errors

	// Get length of error message and use it to get the error message into a char array
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &value);
	GLchar *infolog = new GLchar[value] {};
	glGetShaderInfoLog(shader, value, nullptr, infolog);

	TextFormat::warn(fmt::format("Error message:\n{}", reinterpret_cast<char*>(infolog)), "Shader Error - " + name);
	delete[] infolog; // Clear error message memory

	return false; // False - there was an error
}

void Shader::InitProgramFile(Program &prog, const std::string &vertexData, const std::string &fragmentData)
{
	enum ShaderData { SD_Vertex, SD_Fragment };

	// Create a vertex + fragment shader, throwing if compilation failed
	const auto Create = [&](ShaderData which, bool isVert) {
		const GLuint shader = CreateShader(
			which == SD_Vertex ? vertexData : fragmentData,
			prog.name + (which == SD_Vertex ? ".vert" : ".frag"),
			isVert
		);
		if (!shader) throw std::runtime_error("Shader compilation failed");
		return shader;
	};

	const GLuint vertex = Create(SD_Vertex, true);
	const GLuint fragment = Create(SD_Fragment, false);

	// Create a new program (delete old one if it already exists)
	if (prog.program) glDeleteProgram(prog.program);
	prog.program = glCreateProgram();

	// Attach both shaders to the program
	glAttachShader(prog.program, vertex);
	glAttachShader(prog.program, fragment);
	glLinkProgram(prog.program);

	// Delete shaders
	glDetachShader(prog.program, vertex);
	glDeleteShader(vertex);
	glDetachShader(prog.program, fragment);
	glDeleteShader(fragment);
}

void Shader::InitProgram(Program &prog)
{
	std::string vertexData, fragmentData;

	// Get vertex and fragment files from program name
	static const std::string shadersFile = game.resourcesFolder + "Shaders\\"; // Directory for shaders

	// Read vertex and fragment files into string
	FileManager::ReadFile(shadersFile + prog.name + ".vert", vertexData, false, true);
	FileManager::ReadFile(shadersFile + prog.name + ".frag", fragmentData, false, true);

	// Create shaders and program using shader files
	InitProgramFile(prog, vertexData, fragmentData);
}

// Capitalize first letter
Shader::Program::Program(const char *shaderName) noexcept : name(shaderName) { name[0] -= static_cast<char>(32); }

Shader::~Shader() noexcept
{
	// Delete all valid programs
	EachProgram([&](Program &prog) {
		if (!prog.program) return;
		glDeleteProgram(prog.program);
	});
}

void Shader::EachProgram(std::function<void(Program&)> each)
{
	// Execute given function for each program
	Program *progs = reinterpret_cast<Program*>(&programs);
	const int numProgs = static_cast<int>(sizeof(ShaderPrograms) / sizeof(Program));
	for (int i = 0; i < numProgs; ++i) each(progs[i]);
}

// -------------------- WorldNoise -------------------- 

WorldNoise::WorldNoise(WorldPerlin::NoiseSpline *splines, std::int64_t *seeds)
{
	WorldPerlin *perlins[] = { &elevation, &flatness, &depth, &temperature, &humidity };
	for (int i = 0; i < NoiseEnums::MAX; ++i) {
		WorldPerlin *perlin = perlins[i];
		perlin->noiseSplines = splines[i];
		perlin->ChangeSeed(seeds[i]);
	}
}
WorldNoise::WorldNoise(WorldPerlin::NoiseSpline *splines)
{
	WorldPerlin *perlins[] = { &elevation, &flatness, &depth, &temperature, &humidity };
	const int perlinsSize = static_cast<int>(Math::size(perlins));
	bool hasSplines = splines != nullptr;
	
	for (int i = 0; i < perlinsSize; ++i) {
		WorldPerlin *perlin = perlins[i];
		if (hasSplines) perlin->noiseSplines = splines[i];
		perlin->ChangeSeed(); 
	}
}

// -------------------- FileManager -------------------- 

void FileManager::GetParentDirectory(std::string &dir) noexcept
{
	dir = dir.substr(std::size_t{}, dir.find_last_of("\\/"));
}
bool FileManager::DirectoryExists(std::string path)
{
	struct stat info;
	return stat(path.c_str(), &info) == 0 && info.st_mode & S_IFDIR;
}
bool FileManager::FileExists(std::string path) 
{
	struct stat info;
	return stat(path.c_str(), &info) == 0 && info.st_mode & S_IFREG;
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

void FileManager::ReadFile(const std::string &filename, std::string &contents, bool message, bool throwing)
{
	// Check if file exists
	if (!FileManager::FileExists(filename)) {
		TextFormat::warn(fmt::format("File {} does not exist!", filename), "Invalid filename");
		if (throwing) throw std::runtime_error("Invalid filename");
		return;
	}

	// Setup file stream (moves to eof with ate bit)
	std::ifstream fileStream(filename, std::ios::binary | std::ios::ate);
	
	// Check if stream is valid
	if (!fileStream.good()) {
		TextFormat::warn(fmt::format("File stream failed to initialize on '{}'", filename), "File read error");
		if (throwing) throw FileManager::FileError("File read error");
		return;
	}

	const std::streampos fileSize = fileStream.tellg(); // Get file size
	fileStream.seekg(std::ios::beg); // Move reader to beginning
	contents.resize(fileSize); // Resize string to fit entire file
	fileStream.read(&contents[0], fileSize); // Read file to contents string

	// Log file read message
	if (message && game.mainLoopActive) TextFormat::log(fmt::format("File '{}' read from disk.", filename));
}

void FileManager::LoadImage(ImageInfo &info, const std::string &filename, const ImageInfo::Format &formatInfo, const ImageInfo::Mipmap &mipmapInfo)
{
	// Create a new texture object
	GLuint tex;
	glGenTextures(1, &tex);

	// Set the newly created texture as the currently bound one
	glActiveTexture(GL_TEXTURE0 + (tex - 1)); // GL_TEXTURE0, GL_TEXTURE1, ...
	glBindTexture(GL_TEXTURE_2D, tex);

	bool doMipMap = mipmapInfo.mipmapParam;

	// Determine pixel colour to show depending on texture coordinates - nearest or combined
	// Optionally determine mipmap level selection method with same options
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, doMipMap ? mipmapInfo.mipmapParam : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Texture coordinate overflow handling
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, formatInfo.wrapParam);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, formatInfo.wrapParam);

	// Determine 'border' colour with clamp wrap setting
	if (formatInfo.wrapParam == GL_CLAMP_TO_BORDER) {
		static const float borderColours[4] { 1.0f, 0.0f, 0.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColours);
	}

	// Load in found image whilst setting data
	if (lodepng::decode(
		info.data, info.width, info.height,
		game.resourcesFolder + filename, formatInfo.imgFormat, formatInfo.bitDepth
	)) throw FileManager::FileError("Image load fail");

	// Save the texture data into the currently bound texture object
	glTexImage2D(GL_TEXTURE_2D, 0, formatInfo.OGLFormat, info.width, info.height, 0, formatInfo.OGLFormat, GL_UNSIGNED_BYTE, info.data.data());

	// Create mipmaps (with bias setting for different LODs) for the texture 
	if (doMipMap) {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, mipmapInfo.bias);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
}
