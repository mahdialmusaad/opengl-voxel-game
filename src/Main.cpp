#include "Application/Game.hpp"
voxel_global game; // Define game global

// GLFW error callback
static void glfw_err_cb(int, const char *description) { formatter::warn(description); }

static void init_libs()
{
	if (!glfwInit()) formatter::init_abort(VOXEL_ERR_GLFW_INIT); // Intialize GLFW library
	glfwSetErrorCallback(glfw_err_cb); // GLFW error callback

	game.init(); // Initialize the game global + calculates window defaults

	// Version hints (OGL core 4.6)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Window position hints
	glfwWindowHint(GLFW_POSITION_X, game.window_xpos);
	glfwWindowHint(GLFW_POSITION_Y, game.window_ypos);

#if defined(VOXEL_APPLE)
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Apple configuration
#endif

	// Create the game window with default settings and check for null
	game.window_ptr = glfwCreateWindow(game.window_width, game.window_height, game.def_title.c_str(), nullptr, nullptr);
	if (!game.window_ptr) formatter::init_abort(VOXEL_ERR_WINDOW_INIT);

	glfwMakeContextCurrent(game.window_ptr); // Set GLFW functions to operate on the newly created window
	
	// Initialize OGL function loader (glad)
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) formatter::init_abort(VOXEL_ERR_LOADER);

	formatter::log("Library inits complete");
}

int main(int argc, char *argv[])
{
	init_libs(); // Initializes GLFW and GLAD and the game global
	main_game_obj::init_game(argc, argv); // Initialize settings, global variables and shader values

	main_game_obj game_object{}; // Creates a main 'game' object
	game_object.main_world_loop(); // Start main game loop

	// 'game' destructor terminates libraries and cleans up world-persistent objects.
}
