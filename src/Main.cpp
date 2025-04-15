#include <Utility/Application.hpp>

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
	const char* csource, *errType, *cseverity;

	switch (source) {
		case GL_DEBUG_SOURCE_API:				csource = "API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:		csource = "Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:	csource = "Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:		csource = "Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION:		csource = "Application"; break;
		case GL_DEBUG_SOURCE_OTHER:				csource = "Other"; break;
	} 
	switch (type) {
		case GL_DEBUG_TYPE_ERROR:               errType = "Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: errType = "Deprecated Behaviour"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  errType = "Undefined Behaviour"; break;
		case GL_DEBUG_TYPE_PORTABILITY:         errType = "Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         errType = "Performance"; break;
		case GL_DEBUG_TYPE_MARKER:              errType = "Marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          errType = "Push Group"; break;
		case GL_DEBUG_TYPE_POP_GROUP:           errType = "Pop Group"; break;
		case GL_DEBUG_TYPE_OTHER:               errType = "Other"; break;
	} 
	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:			cseverity = "High"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:			cseverity = "Medium"; break;
		case GL_DEBUG_SEVERITY_LOW:				cseverity = "Low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:	cseverity = "Notification"; break;
	}

	TextFormat::log(
		std::format("{} - [{}][{}][{}][{}]", message, csource, errType, cseverity, id)
	);
}
static void GLErrorCallback(int err, const char* des)
{
	if (!game.mainLoopActive) return;
	TextFormat::log(std::format("OGL error (ID {}): {}", err, des));
}

// OpenGL I/O callbacks can only be set as global functions, 
// so create wrappers for the actual functions defined in the 'callbacks' struct instead
Badcraft::Callbacks* cb;
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
		exit(-1);
	}

	TextFormat::log("GLFW initialized");

	// Window hints (OGL comp. 4.6)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	// Compatibility configuration for Apple operating systems
	#ifdef __APPLE__ 
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif

	// Create the game window with default settings into global
	game.window = glfwCreateWindow(1800, 1200, "Loading...", nullptr, nullptr);

	// Check if window failed to create
	if (game.window == nullptr) {
		TextFormat::warn("Window creation failed", "Window");
		glfwTerminate();
		exit(-1);
	}

	TextFormat::log("Window created");

	// Set GLFW functions to operate on the newly created window
	glfwMakeContextCurrent(game.window);

	// Initialize OGL function loader
	const GLADloadproc loader = reinterpret_cast<GLADloadproc>(glfwGetProcAddress);
	if (!gladLoadGLLoader(loader)) {
		TextFormat::warn("GLAD initialization failed", "GLAD fail");
		exit(-1);
	}

	TextFormat::log("GLAD loaded");

	// Utility functions
	glfwSetWindowSizeLimits(game.window, 1800, 1200, GLFW_DONT_CARE, GLFW_DONT_CARE); // Enforce minimum window size
	glfwSetInputMode(game.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Disable moving the cursor so it doesn't go outside the window
	glfwSetWindowPos(game.window, 2000, 860); // debug

	game.shader.InitShader(); // Initialize shader class

	// Load game textures and get information about them (width, height, etc)
	Shader::LoadTexture(game.blocksTextureInfo, "src/Resources/atlas.png", false, GL_NEAREST, false);
	Shader::LoadTexture(game.textTextureInfo, "src/Resources/textAtlas.png", true, GL_NEAREST, false);
	Shader::LoadTexture(game.inventoryTextureInfo, "src/Resources/inventory.png", false, GL_NEAREST, false);
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

int main()
{
	// Initialize game (load libraries, shaders, textures, etc)
	Initialize();
	
	// Create application class
	Badcraft bc = Badcraft();
	cb = &bc.callbacks;

	// Set all input and debug callbacks
	SetCallbacks();

	// Start main game loop
	bc.Main();

	// Optional return
	return 0;
}
