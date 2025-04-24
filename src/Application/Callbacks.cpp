#include "Application.hpp"

// Although the 'Callbacks' struct is also part of the 'Badcraft' class (seen in the Application.hpp),
// they are still two distinct objects and including both the class and the inner struct would make
// the Application.cpp file very large, a pain to debug and scroll through and take long to compile.

// The 'Callbacks' struct still needs to be inside the Application header as it requires access to its members
// and functions (e.g. resizing window needs to update the perspective matrix, which is in the Badcraft class)

Badcraft::Callbacks::Callbacks()
{
	glfwGetWindowPos(game.window, &game.windowX, &game.windowY);
	glfwGetWindowSize(game.window, &game.width, &game.height);
}

// The window is already defined, so no need for the first argument
void Badcraft::Callbacks::KeyPressCallback(int key, int, int action, int)
{
	// Chat is handled by CharCallback
	if (game.chatting) {
		if (action == GLFW_RELEASE) return;

		static const auto ExitChat = [&]() {
			// Hide the text object and empty the inputted message
			app->world.textRenderer.ChangeText(app->m_chatText, "");
			app->m_chatText->type = TextRenderer::T_Type::Hidden;
			game.chatting = false; // Exit chat mode
		};

		// See if they want to confirm chat message/command rather than adding more text to it
		if (key == GLFW_KEY_ENTER) {
			ApplyChat();
			ExitChat();
		}
		// Exit chat if escape is pressed
		else if (key == GLFW_KEY_ESCAPE) {
			ExitChat();
		}
		// Chat message editing is done by CharCallback but it does not include backspace
		else if (key == GLFW_KEY_BACKSPACE) {
			std::string chatText = app->m_chatText->GetText();
			if (chatText.size()) { // Check if there is any text present
				chatText.erase(chatText.end() - 1); // Remove last char
				app->world.textRenderer.ChangeText(app->m_chatText, chatText);
			}
		}
		// Set current message to the previous one (if its valid)
		else if (key == GLFW_KEY_UP && previousChat.size()) {
			app->world.textRenderer.ChangeText(app->m_chatText, previousChat);
		}
		
		return;
	}

	// Determine if the input has a linked function and execute if so
	ApplyInput(key, action);
}

void Badcraft::Callbacks::CharCallback(unsigned codepoint)
{
	// Add inputted char into chat message
	if (game.chatting) {
		app->world.textRenderer.ChangeText(
			app->m_chatText,
			app->m_chatText->GetText() + narrow_cast<char>(codepoint)
		);
	}
}

void Badcraft::Callbacks::MouseClickCallback(int button, int action, int) noexcept
{
	// Re-capture mouse as some actions can free the mouse
	if (!app->player.inventoryOpened) glfwSetInputMode(game.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Check if the mouse button is being pressed
	if (action == GLFW_PRESS) {
		// Left mouse to attempt block breaking,
		if (button == GLFW_MOUSE_BUTTON_LEFT) app->playerFunctions.BreakBlock();
		// right mouse to attempt block placing
		else if (button == GLFW_MOUSE_BUTTON_RIGHT) app->playerFunctions.PlaceBlock();
	}
}

void Badcraft::Callbacks::ScrollCallback(double, double yoffset) noexcept
{
	// Scroll through inventory
	app->playerFunctions.UpdateScroll(static_cast<float>(yoffset));
}

void Badcraft::Callbacks::ResizeCallback(int width, int height) noexcept
{
	// No need to update or recalculate if the game has just been minimized,
	// lower the FPS instead to decrease unecessary usage
	if (width <= 0 || height <= 0) {
		game.minimized = true;
		glfwSwapInterval(10); // Update every 10 screen frames
		return;
	}
	else if (game.minimized) {
		game.minimized = false;
		// Set to default game frame update state
		glfwSwapInterval(app->maxFPS ? 0 : 1);
	}

	// Update graphics viewport and settings
	glViewport(0, 0, width, height);
	game.width = width;
	game.height = height;

	// Scale GUI to accomodate the new window size
	app->UpdateAspect();
}

void Badcraft::Callbacks::MouseMoveCallback(double xpos, double ypos) noexcept
{
	// Avoid large jumps in camera direction when window gains focus
	if (game.focusChanged) {
		game.mouseX = xpos;
		game.mouseY = ypos;
		game.focusChanged = false;
	}
	else {
		// Move player camera
		app->playerFunctions.MouseMoved(xpos - game.mouseX, game.mouseY - ypos);
		game.mouseX = xpos; game.mouseY = ypos;
	}
}

void Badcraft::Callbacks::WindowMoveCallback(int x, int y) noexcept
{
	// Store window location for information purposes
	game.windowX = x;
	game.windowY = y;
}

void Badcraft::Callbacks::CloseCallback() noexcept
{
	// Exit out of main game loop
	game.mainLoopActive = false;
}

void Badcraft::Callbacks::TakeScreenshot() noexcept
{
	bool error = false;
	constexpr const char* screenshotfolder = "Screenshots";

	// Ensure screenshot folder is valid by either creating or replacing
	try {
		// Create if there is no folder
		if (!std::filesystem::exists(screenshotfolder)) std::filesystem::create_directory(screenshotfolder);
		// Ensure folder is valid
		else if (!std::filesystem::is_directory(screenshotfolder)) {
			std::filesystem::remove(screenshotfolder);
			std::filesystem::create_directory(screenshotfolder);
		}
	}
	catch (std::filesystem::filesystem_error e) { error = true; }

	const char* newScreenshotText = "Failed to save screenshot";

	if (!error) {
		constexpr const std::size_t snprintfsz = static_cast<std::size_t>(200);
		char filenamebuf[snprintfsz], directorybuf[snprintfsz];

		// Lodepng needs pixel array in std::vector - 3 per pixel for R,G,B colours
		const unsigned w = game.width, h = game.height;
		std::vector<unsigned char> pixels(3u * w * h);

		// Read pixels from screen to vector
		glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

		// Get time in milliseconds (avoid name conflicts when taking lots of screenshots)
		const double timeDouble = glfwGetTime();
		const int milliseconds = static_cast<int>((timeDouble - floor(timeDouble)) * 1000.0f);

		// Get *thread-safe* formatted time values (made complicated by different operating systems) - prevent unsafe warning
		std::tm timeValues;
		std::time_t timeSecs;

		#if defined(__unix__)
			localtime_r(&timeSecs, &timeValues);
		#elif defined(_MSC_VER)
			localtime_s(&timeValues, &timeSecs);
		#else
			static std::mutex mtx;
			std::lock_guard<std::mutex> lock(mtx);
			timeValues = *std::localtime(&timeSecs);
		#endif

		// File name for displaying and creating directory path
		snprintf(filenamebuf, snprintfsz, "Screenshot %d-%d-%d %d-%02d-%02d-%03d.png",
			timeValues.tm_year + 1900, timeValues.tm_mon + 1, timeValues.tm_mday, timeValues.tm_hour, timeValues.tm_min, timeValues.tm_sec, milliseconds);

		// Get full path to save to
		snprintf(directorybuf, snprintfsz, "%s/%s", screenshotfolder, filenamebuf); 

		// Flip Y axis as OGL uses opposite Y coordinates (bottom-top instead of top-bottom)
		std::vector<unsigned char> newHalf(3u * w * h);
		for (unsigned x = 0; x < w; ++x) {
			for (unsigned y = 0; y < h; ++y) {
				for (int s = 0; s < 3; ++s) {
					int start = (x + y * w) * 3 + s;
					int end = (x + (h - 1 - y) * w) * 3 + s;
					newHalf[start] = pixels[end];
				}
			}
		}

		// Save to PNG file in directory constructed above (RGB colours, no alpha)
		const bool success = lodepng::encode(directorybuf, newHalf, w, h, LodePNGColorType::LCT_RGB) == 0;

		newHalf.clear();
		pixels.clear();

		// Create result text to be displayed
		if (success) {
			// Overwrite directory buffer to avoid having to create another buffer
			snprintf(directorybuf, snprintfsz, "Screenshot saved as %s", filenamebuf);
			newScreenshotText = static_cast<const char*>(directorybuf);
		}
	}

	// Update text with results
	app->world.textRenderer.ChangeText(app->m_screenshotText, newScreenshotText);
	app->m_screenshotText->type = TextRenderer::T_Type::TemporaryShow;
	app->m_screenshotText->ResetTextTime();
}

void Badcraft::Callbacks::ToggleInventory() noexcept
{
	// Open or close the inventory
	b_not(app->player.inventoryOpened);
	glfwSetInputMode(game.window, GLFW_CURSOR, app->player.inventoryOpened ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	// Reset mouse position
	glfwSetCursorPos(game.window, static_cast<double>(game.width) / 2.0, static_cast<double>(game.height) / 2.0);
	game.focusChanged = true;
}

void Badcraft::Callbacks::ApplyInput(int key, int action) noexcept
{
	constexpr int repeatableInput = GLFW_PRESS | GLFW_REPEAT;
	constexpr int pressInput = GLFW_PRESS;

	typedef std::pair<int, std::function<void()>> pair;
	static const std::unordered_map<int, pair> keyFunctionMap = {

		// Single-press inputs

		{ GLFW_KEY_ESCAPE, { pressInput, [&]() {
			game.mainLoopActive = false;
		}}},
		{ GLFW_KEY_E, { pressInput, [&]() {
			ToggleInventory();
		}}},
		{ GLFW_KEY_Z, { pressInput, [&]() {
			glPolygonMode(GL_FRONT_AND_BACK, b_not(app->wireframe) ? GL_LINE : GL_FILL);
		}}},
		{ GLFW_KEY_X, { pressInput, [&]() {
			glfwSwapInterval(!b_not(app->maxFPS));
		}}},
		{ GLFW_KEY_C, { pressInput, [&]() {
			b_not(app->player.collisionEnabled);
		}}},
		{ GLFW_KEY_V, { pressInput, [&]() {
			b_not(game.noGeneration);
		}}},
		{ GLFW_KEY_R, { pressInput, [&]() {
			game.shader.InitShader();
			app->UpdateAspect();
		}}},
		{ GLFW_KEY_F1, { pressInput, [&]() {
			b_not(app->showGUI);
		}}},
		{ GLFW_KEY_F2, { pressInput, [&]() {
			TakeScreenshot();
		}}},
		{ GLFW_KEY_F3, { pressInput, [&]() {
			glfwSetInputMode(game.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			game.focusChanged = true;
		}}},
		{ GLFW_KEY_SLASH, { pressInput, [&]() {
			app->showGUI = true;
			BeginChat();
		}}},
		{ GLFW_KEY_LEFT_BRACKET, { pressInput, [&]() {
			app->world.UpdateRenderDistance(app->world.chunkRenderDistance + 1);
		}}},
		{ GLFW_KEY_RIGHT_BRACKET, { pressInput, [&]() {
			app->world.UpdateRenderDistance(app->world.chunkRenderDistance - 1);
		}}},

		// Repeatable inputs

		{ GLFW_KEY_COMMA, { repeatableInput, [&]() {
			app->player.defaultSpeed += 1.0f;
		}}},
		{ GLFW_KEY_PERIOD, { repeatableInput, [&]() {
			app->player.defaultSpeed -= 1.0f;
		}}},
		{ GLFW_KEY_I, { repeatableInput, [&]() {
			app->player.fov += Math::TORADIANS_FLT;
			app->UpdatePerspective();
		}}},
		{ GLFW_KEY_O, { repeatableInput, [&]() {
			app->player.fov -= Math::TORADIANS_FLT;
			app->UpdatePerspective();
		}}}
	};

	// Add to input information list
	if (action == GLFW_RELEASE) game.keyboardState.erase(key);
	else game.keyboardState.insert({ key, action });
	game.anyKeyPressed = game.keyboardState.size() > 0;

	// Check if key is listed in input map
	const auto& keyboardPair = keyFunctionMap.find(key);

	if (keyboardPair != keyFunctionMap.end()) {
		const auto& [inputAction, keyFunction] = keyboardPair->second;
		// Check if input action matches, run the associated function if it is
		if ((action & inputAction) != 0) keyFunction();
	}
}

void Badcraft::Callbacks::BeginChat() noexcept
{
	app->m_chatText->type = TextRenderer::T_Type::Default;
	game.chatting = true;
}

void Badcraft::Callbacks::ApplyChat()
{
	// Check if there is any text in the first place
	std::string& command = app->m_chatText->GetText();

	const int cmdSize = static_cast<int>(command.size());
	if (!cmdSize) return; // Do nothing if empty

	// Save the current non-empty message (could implement list instead of just saving one message only)
	previousChat = command;

	if (command[0] != '/') {
		// TODO: normal chat
		return;
	}

	// Ensure there is only one space at the end of the command
	for (int i = cmdSize - 1; i >= 0; --i) {
		if (command[i] != ' ') {
			std::size_t rem = static_cast<std::size_t>(cmdSize - i - 1);
			command = command.substr(0, command.size() - rem);
			command += " ";
			break;
		}
	}

	// Get each argument from the command given using spaces
	std::vector<std::string> args;
	std::size_t last = static_cast<std::size_t>(1);

	while (true) {
		std::size_t ind = command.find_first_of(" ", last);
		if (ind == std::string::npos) break;
		args.emplace_back(command.substr(last, ind - last));
		last = ind + 1;
	}
	if (args.size() == 0) return;

	// Different string conversion functions
	enum class CONV {
		Int,
		Object,
		Double
	};

	// Quick/simple variable shortcuts
	std::vector<std::string> &a = args;
	PlayerObject &plr = app->player;
	Player &fplayer = app->playerFunctions;
	glm::dvec3 &pos = plr.position;

	// For debug purposes
	int currentArgumentCheck = 0;
	std::string currentArgument;
	bool isConverting = false;

	const auto getArg = [&](int ind) -> std::string& {
		std::string& arg = a[ind];
		currentArgumentCheck = ind;
		currentArgument = arg;
		return arg;
	};

	const auto getInt = [&](int ind) { return std::stoi(getArg(ind)); };
	const auto getBlk = [&](int ind) { return narrow_cast<ObjectID>(std::stoi(getArg(ind))); };
	const auto getDbl = [&](int ind) { return std::stod(getArg(ind)); };

	enum class CVALS { X, Y, Z };
	typedef std::pair<int, CVALS> ConversionPair;
	// Convert specific terms into their associated variables (e.g. '~' becomes a specific position axis)
	static const auto cvrt = [&](std::vector<ConversionPair> indpairs) {
		isConverting = true;
		for (ConversionPair& pair : indpairs) {
			const auto &[ind, axis] = pair;
			std::string& c_arg = getArg(ind);

			bool isSquigglyCharacterThing = c_arg.size() == 1 && c_arg[0] == '~';
			if (!isSquigglyCharacterThing) continue;

			switch (axis) {
				case CVALS::X:
					c_arg = std::to_string(pos.x);
					break;
				case CVALS::Y:
					c_arg = std::to_string(pos.y);
					break;
				case CVALS::Z:
					c_arg = std::to_string(pos.z);
					break;
				default:
					break;
			}
		}

		isConverting = false;
	};

	// Macro for argument size check
	#define argnum(x) if (args.size() != x) return;

	// Map of all commands and their functions
	typedef std::function<void()> func;
	static const std::unordered_map<std::string, func> commandsMap = {
		{ "tp", [&]() {
			argnum(3);
			cvrt({ { 0, CVALS::X }, { 1, CVALS::Y }, { 2, CVALS::Z } });
			fplayer.SetPosition(glm::dvec3(getDbl(0), getDbl(1), getDbl(2)));
		}},
		{ "exit", [&]() {
			game.mainLoopActive = false;
		}},
		{ "help", [&]() {
			game.mainLoopActive = false; // TODO: do something useful instead of quitting
		}},
		{ "speed", [&]() {
			argnum(1);
			plr.defaultSpeed = getDbl(0);
		}},
		{ "testval", [&]() {
			argnum(2);
			game.testfloat = getDbl(0);
			game.testfloat2 = getDbl(1);
			fplayer.UpdateInventory();
		}},
		{ "tick", [&]() {
			argnum(1);
			game.tickSpeed = getDbl(0);
		}},
		{ "time", [&]() {
			argnum(1);
			const double newTime = getDbl(0);
			game.currentFrameTime = newTime;
			game.tickedFrameTime = newTime;
		}},
		{ "fill", [&]() {
			argnum(7);
			cvrt({ {0, CVALS::X}, {1, CVALS::Y}, {2, CVALS::Z}, {3, CVALS::X}, {4, CVALS::Y}, {5, CVALS::Z} });
			app->world.FillBlocks(
				getInt(0), getInt(1), getInt(2),
				getInt(3), getInt(4), getInt(5),
				getBlk(6)
			);
		}},
		{ "dcmp", [&]() {
			argnum(1);
			std::vector<char> binary(static_cast<std::size_t>(48 * 1024));
			GLenum format = 0; GLint length = 0;
			glGetProgramBinary(getInt(0), 20480, &length, &format, binary.data());
			std::ofstream("bin.txt").write(binary.data(), length);
		}}
	};

	#undef argnum

	const auto commandKeyFind = commandsMap.find(args[0]); // Find the command definition
	if (commandKeyFind != commandsMap.end()) { // Check if command exists
		args.erase(args.begin()); // Erase default command so args[0] is the first command argument
		try { commandKeyFind->second(); } // Execute linked function
		catch (std::exception err) {
			std::stringstream errfmt;
			errfmt << "Error during command parsing:\nArgument index: " << currentArgumentCheck << "\nArgument: " << currentArgument;
			TextFormat::warn(errfmt.str(), err.what());
		}
	}
}
