#include "Application/Game.hpp"

// Define extern variables - in main.cpp to avoid redefinition errors
GameGlobal game;
ChunkLookupData chunkLookupData[ChunkValues::uniqueFaces];

// Debug OpenGL functions for errors
static void GLDebugOutput( 
	GLenum source, 
	GLenum type, 
	unsigned id, 
	GLenum severity, 
	GLsizei, 
	const char *message, 
	const void*
) {
	if (id == 131185 || id == 131154) return;
	std::string sourcestr, typestr, severitystr;

	switch (source) {
		case GL_DEBUG_SOURCE_API:               sourcestr = "[API]"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:     sourcestr = "[Window]"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:   sourcestr = "[Shader]"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:       sourcestr = "[External]"; break;
		case GL_DEBUG_SOURCE_APPLICATION:       sourcestr = "[Application]"; break;
		case GL_DEBUG_SOURCE_OTHER:             sourcestr = "[Other]"; break;
	} 
	switch (type) {
		case GL_DEBUG_TYPE_ERROR:               typestr = "[Error]"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typestr = "[Deprecated]"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typestr = "[Undefined]"; break;
		case GL_DEBUG_TYPE_PORTABILITY:         typestr = "[Portability]"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         typestr = "[Performance]"; break;
		case GL_DEBUG_TYPE_MARKER:              typestr = "[Marker]"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          typestr = "[Push]"; break;
		case GL_DEBUG_TYPE_POP_GROUP:           typestr = "[Pop]"; break;
		case GL_DEBUG_TYPE_OTHER:               typestr = "[Other]"; break;
	} 
	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:            severitystr = "[High]"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:          severitystr = "[Medium]"; break;
		case GL_DEBUG_SEVERITY_LOW:             severitystr = "[Low]"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:    severitystr = "[Notification]"; break;
	}

	TextFormat::warn(fmt::format("{} - {}{}{} - ID {}", message, sourcestr, typestr, severitystr, id), "OGL debug message");
}

static void GLErrorCallback(int err, const char *description)
{
	TextFormat::warn(description, fmt::format("OGL error {}", err));
}

// OpenGL I/O callbacks can only be set as global functions -
// use wrappers for the actual functions defined in the 'Callbacks' struct
GameObject::Callbacks *callbacks;
static void CharCallback(GLFWwindow*, unsigned codepoint) { callbacks->CharCallback(codepoint); }
static void ResizeCallback(GLFWwindow*, int width, int height) { callbacks->ResizeCallback(width, height); }
static void KeyPressCallback(GLFWwindow*, int key, int scancode, int action, int mods) { callbacks->KeyPressCallback(key, scancode, action, mods); }
static void MouseClickCallback(GLFWwindow*, int button, int action, int mods) { callbacks->MouseClickCallback(button, action, mods); }
static void ScrollCallback(GLFWwindow*, double xoffset, double yoffset) { callbacks->ScrollCallback(xoffset, yoffset); }
static void MouseMoveCallback(GLFWwindow*, double xpos, double ypos) { callbacks->MouseMoveCallback(xpos, ypos); }
static void WindowMoveCallback(GLFWwindow*, int x, int y) { callbacks->WindowMoveCallback(x, y); }
static void CloseCallback(GLFWwindow*) { callbacks->CloseCallback(); }

static void Initialize()
{
	// Intialize GLFW library
	if (!glfwInit()) throw std::runtime_error("GLFW initialization failure");

	// GLFW error callback
	glfwSetErrorCallback(GLErrorCallback);
	
	// Window hints (OGL core 4.6)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Compatibility configuration
	#ifdef __APPLE__ 
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif

	// Init the game global and create the game window with default settings
	game.Init();
	game.window = glfwCreateWindow(game.width, game.height, "Loading...", nullptr, nullptr);

	// Check if window failed to create
	if (!game.window) {
		glfwTerminate();
		throw std::runtime_error("Window creation failure");
	}

	// Set GLFW functions to operate on the newly created window
	glfwMakeContextCurrent(game.window);
	
	// Initialize OGL function loader
	if (!gladLoadGL(glfwGetProcAddress)) {
		glfwDestroyWindow(game.window);
		glfwTerminate();
		throw std::runtime_error("GLAD load failure");
	}

	TextFormat::log("Library inits complete");
}

static void SetCallbacks(GameObject::Callbacks &callbacksObj) 
{
	// Set window event callbacks
	GLFWwindow *window = game.window;
	callbacks = &callbacksObj;

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
	glDebugMessageCallback(GLDebugOutput, nullptr);
}

int main(int, char *argv[])
{
	Initialize(); // Initializes GLFW and GLAD
	GameObject::InitGame(argv[0]); // Initializes game variables and textures
	
	{
		GameObject gameObject{}; // Creates a main 'game' object
		SetCallbacks(gameObject.callbacks); // Set all input and debug callbacks
		gameObject.Main(); // Start main game loop
	}
	// Out of scope - destructors are called to delete created buffers and objects.

	game.Cleanup(); // Clean up some member variables and delete buffers defined in game global
	glfwTerminate(); // Terminate GLFW - destroys game window as well
}
