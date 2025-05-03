#include "Application/Application.hpp"

// Initialize globals from Globals/Definitions.hpp
GameGlobalsObject game{};
ChunkLookupTable chunkLookupData{};

// Debug OpenGL functions for errors
static void GLDebugOutput(
	GLenum source, 
	GLenum type, 
	unsigned id, 
	GLenum severity, 
	GLsizei, 
	const char* message, 
	const void*
) {
	if (id == 131185 || id == 131154) return;
	std::string sourcestr, typestr, severitystr;
	
	switch (source) {
		case GL_DEBUG_SOURCE_API:				sourcestr = "[API]"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:		sourcestr = "[Window System]"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:	sourcestr = "[Shader Compiler]"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:		sourcestr = "[Third Party]"; break;
		case GL_DEBUG_SOURCE_APPLICATION:		sourcestr = "[Application]"; break;
		case GL_DEBUG_SOURCE_OTHER:				sourcestr = "[Other]"; break;
	} 
	switch (type) {
		case GL_DEBUG_TYPE_ERROR:               typestr = "[Error]"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typestr = "[Deprecated Behaviour]"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typestr = "[Undefined Behaviour]"; break;
		case GL_DEBUG_TYPE_PORTABILITY:         typestr = "[Portability]"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         typestr = "[Performance]"; break;
		case GL_DEBUG_TYPE_MARKER:              typestr = "[Marker]"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          typestr = "[Push Group]"; break;
		case GL_DEBUG_TYPE_POP_GROUP:           typestr = "[Pop Group]"; break;
		case GL_DEBUG_TYPE_OTHER:               typestr = "[Other]"; break;
	} 
	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:			severitystr = "[High]"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:			severitystr = "[Medium]"; break;
		case GL_DEBUG_SEVERITY_LOW:				severitystr = "[Low]"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:	severitystr = "[Notification]"; break;
	}

	TextFormat::log(fmt::format("{} - {}{}{} - ID {}", message, sourcestr, typestr, severitystr, id));
}
static void GLErrorCallback(int err, const char* description)
{
	if (!game.mainLoopActive) return;
	TextFormat::log(fmt::format("OGL error {}: {}", err, description));
}

// OpenGL I/O callbacks can only be set as global functions, 
// so create wrappers for the actual functions defined in the 'callbacks' struct instead
GameObject::Callbacks* cb;
static void CharCallback(GLFWwindow*, unsigned codepoint)
{
	cb->CharCallback(codepoint);
}
static void ResizeCallback(GLFWwindow*, int width, int height)
{ 
	cb->ResizeCallback(width, height);
}
static void KeyPressCallback(GLFWwindow*, int key, int scancode, int action, int mods)
{ 
	cb->KeyPressCallback(key, scancode, action, mods); 
}
static void MouseClickCallback(GLFWwindow*, int button, int action, int mods)
{ 
	cb->MouseClickCallback(button, action, mods); 
}
static void ScrollCallback(GLFWwindow*, double xoffset, double yoffset)
{ 
	cb->ScrollCallback(xoffset, yoffset); 
}
static void MouseMoveCallback(GLFWwindow*, double xpos, double ypos)
{ 
	cb->MouseMoveCallback(xpos, ypos); 
}
static void WindowMoveCallback(GLFWwindow*, int x, int y)
{ 
	cb->WindowMoveCallback(x, y); 
}
static void CloseCallback(GLFWwindow*)
{ 
	cb->CloseCallback(); 
}

static void Initialize()
{
	// Intialize GLFW library
	if (!glfwInit()) {
		TextFormat::warn("GLFW initialization failed", "Init failed");
		throw std::runtime_error("GLFW failure");
	}
	
	TextFormat::log("GLFW initialized");

	// Window hints (OGL core 4.6)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Compatibility configuration for Apple operating systems
	#ifdef __APPLE__ 
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif

	// Create the game window with default settings into global
	constexpr int width = 1200, height = 800;
	game.window = glfwCreateWindow(width, height, "Loading...", nullptr, nullptr);

	// Check if window failed to create
	if (game.window == nullptr) {
		TextFormat::warn("Window creation failed", "Null window");
		glfwTerminate();
		throw std::runtime_error("Window failure");
	}

	TextFormat::log("Window created");

	// Set GLFW functions to operate on the newly created window
	glfwMakeContextCurrent(game.window);
	
	// Initialize OGL function loader
	if (!gladLoadGL(glfwGetProcAddress)) {
		TextFormat::warn("GLAD initialization failed", "GLAD fail");
		glfwDestroyWindow(game.window);
		glfwTerminate();
		throw std::runtime_error("GLAD failure");
	}

	TextFormat::log("GLAD loaded");

	// Utility functions
	glfwSetWindowSizeLimits(game.window, 300, 200, GLFW_DONT_CARE, GLFW_DONT_CARE); // Enforce minimum window size
	glfwSetInputMode(game.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Disable moving the cursor so it doesn't go outside the window
	glfwSwapInterval(1); // Swap buffers with screen's refresh rate

	// Attempt to find resources folder - located in the same directory as exe
	const std::string resFolder = game.currentDirectory + "\\Resources\\";
	if (std::filesystem::exists(resFolder)) game.resourcesFolder = resFolder;
	else throw std::runtime_error("Resources folder not found");

	game.shader.InitShader(); // Initialize shader class

	// Load game textures and get information about them (width, height, etc)
	Shader::LoadTexture(game.blocksTextureInfo, "Textures\\atlas.png", false, GL_NEAREST, false);
	Shader::LoadTexture(game.textTextureInfo, "Textures\\textAtlas.png", true, GL_NEAREST, false);
	Shader::LoadTexture(game.inventoryTextureInfo, "Textures\\inventory.png", false, GL_NEAREST, false);
}

static void SetCallbacks() 
{
	// Set window event callbacks
	GLFWwindow* window = game.window;
	glfwSetFramebufferSizeCallback(window, ResizeCallback);
	glfwSetMouseButtonCallback(window, MouseClickCallback);
	glfwSetWindowPosCallback(window, WindowMoveCallback);
	glfwSetCursorPosCallback(window, MouseMoveCallback);
	glfwSetWindowCloseCallback(window, CloseCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	glfwSetKeyCallback(window, KeyPressCallback);
	glfwSetCharCallback(window, CharCallback);

	// Enable debug messages for OpenGL errors
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

	// Error/debug callbacks
	glDebugMessageCallback(GLDebugOutput, nullptr);
	glfwSetErrorCallback(GLErrorCallback);
}

int main(int, char *argv[])
{
	// Get current directory for finding resources and shaders folders
	game.currentDirectory = TextFormat::getParentDirectory(argv[0]);
	
	// Initialize game (load libraries, shaders, textures, etc)
	Initialize();
	
	// Create application class
	GameObject bc = GameObject();
	cb = &bc.callbacks;

	// Set all input and debug callbacks
	SetCallbacks();

	// Start main game loop
	bc.Main();

	// Optional return
	return 0;
}
