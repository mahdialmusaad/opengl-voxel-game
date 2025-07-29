#include "Game.hpp"

// Although the 'Callbacks' struct is also part of the 'GameObject' class (seen in the Application.hpp),
// they are still two distinct objects and including both the class and the 'Callbacks' struct would make
// the other .cpp file very large, a pain to debug and scroll through and take a long time to compile.

// The 'Callbacks' struct still needs to be inside the Application header as it requires access to its members
// and functions (e.g. resizing window needs to update the perspective matrix, which is in the GameObject class)

GameObject::Callbacks::Callbacks(GameObject *appPtr) : m_app(appPtr)
{
	glfwGetWindowPos(game.window, &game.windowX, &game.windowY);
	glfwGetWindowSize(game.window, &game.width, &game.height);
}

// The window is already defined, so no need for the first argument
void GameObject::Callbacks::KeyPressCallback(int key, int, int action, int)
{
	// Adding text to chat is handled by CharCallback
	if (game.chatting) {
		if (action == GLFW_RELEASE) {
			game.keyboardState.erase(GLFW_KEY_SLASH);
			return;
		}

		const auto ExitChat = [&](TextRenderer::TextType chatType) {
			m_app->world.textRenderer.ChangeText(m_app->m_commandText, ""); // Empty player input
			m_app->m_commandText->textType = TextRenderer::TextType::Hidden; // Hide command text
			m_app->m_chatText->textType = chatType; // Set chat text type
			game.chatting = false; // Exit chat mode
			m_chatHistorySelector = -1; // Rest chat history selector
		};

		// See if the player want to confirm chat message/command rather than adding more text to it
		if (key == GLFW_KEY_ENTER) {
			ApplyCommand(); // Either add player message or execute command
			// Chat log temporarily stays visible if the player actually typed something
			if (m_app->m_commandText->GetText().size()) {
				ExitChat(m_app->m_chatText->textType = TextRenderer::TextType::TemporaryShow);
				m_app->m_chatText->ResetTextTime();
			} else ExitChat(TextRenderer::TextType::Hidden);
		}
		// Exit chat if escape is pressed - hide chat instantly
		else if (key == GLFW_KEY_ESCAPE) ExitChat(TextRenderer::TextType::Hidden);
		// Chat message editing is done by CharCallback but it does not include backspace
		else if (key == GLFW_KEY_BACKSPACE) {
			std::string chatText = m_app->m_commandText->GetText();
			if (chatText.size()) { // Check if there is any text present
				chatText.erase(chatText.end() - 1); // Remove last char
				m_app->world.textRenderer.ChangeText(m_app->m_commandText, chatText);
			}
		}
		// Set current message to the previous one if there is one
		else if (key == GLFW_KEY_UP && m_previousChats.size() > static_cast<std::size_t>(m_chatHistorySelector + 1)) {
			m_app->world.textRenderer.ChangeText(m_app->m_commandText, m_previousChats[++m_chatHistorySelector]);
		}
		// Set current message to the next one if the selector is more than -1 (default text if going to present)
		else if (key == GLFW_KEY_DOWN && m_chatHistorySelector > -1) {
			const std::string newCommandText = --m_chatHistorySelector == -1 ? (game.isChatCommand ? "/" : "") : m_previousChats[m_chatHistorySelector];
			m_app->world.textRenderer.ChangeText(m_app->m_commandText, newCommandText);
		}
		// Handle text pasting
		else if (game.holdingCtrl && key == GLFW_KEY_V) AddCommandText(glfwGetClipboardString(game.window));
	}
	// Determine if the input has a linked function and execute if so 
	else ApplyInput(key, action);
}

void GameObject::Callbacks::CharCallback(unsigned codepoint)
{
	// Ignore unwanted initial input
	if (game.ignoreChatStart) {
		game.ignoreChatStart = false;
		return;
	}

	// Add inputted char into chat message
	if (game.chatting) {
		const char character = static_cast<char>(codepoint);
		if (game.holdingCtrl && (character == 'v' || character == 'V')) return; // CTRL+V is handled elsewhere
		else AddCommandText(std::string(1, character));
	}
}

void GameObject::Callbacks::MouseClickCallback(int button, int action, int) noexcept
{
	if (game.chatting) return;
	// Re-capture mouse as some actions can free the mouse
	if (!m_app->player.inventoryOpened) glfwSetInputMode(game.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Check if the mouse button is being pressed
	if (action == GLFW_PRESS) {
		// Left mouse to attempt block breaking,
		if (button == GLFW_MOUSE_BUTTON_LEFT) m_app->playerFunctions.BreakBlock();
		// right mouse to attempt block placing
		else if (button == GLFW_MOUSE_BUTTON_RIGHT) m_app->playerFunctions.PlaceBlock();
	}
}

void GameObject::Callbacks::ScrollCallback(double, double yoffset) noexcept
{
	if (game.chatting) return;
	// Scroll through inventory
	m_app->playerFunctions.UpdateScroll(static_cast<float>(yoffset));
}

void GameObject::Callbacks::ResizeCallback(int width, int height) noexcept
{
	// No need to update or recalculate if the game has just been minimized,
	// lower the FPS instead to decrease unecessary usage
	if (width + height <= 0) {
		game.minimized = true;
		glfwSwapInterval(game.refreshRate);
		return;
	}
	else if (game.minimized) {
		game.minimized = false;
		glfwSwapInterval(static_cast<int>(game.vsyncFPS)); // Set to default game frame update state
	}

	// Update graphics viewport and settings
	glViewport(0, 0, width, height);

	// Save previous dimensions for exiting out of fullscreen
	if (game.fullscreen) {
		game.fwidth = game.width;
		game.fheight = game.height;
	}

	game.width = width;
	game.height = height;

	// Scale GUI to accomodate the new window size
	m_app->UpdateAspect();
}

void GameObject::Callbacks::MouseMoveCallback(double xpos, double ypos) noexcept
{
	if (game.chatting) return;

	// Avoid large jumps in camera direction when window gains focus
	if (game.focusChanged) {
		game.mouseX = xpos;
		game.mouseY = ypos;
		game.focusChanged = false;
	}
	else {
		// Move player camera
		m_app->playerFunctions.UpdateCameraDirection(xpos - game.mouseX, game.mouseY - ypos);
		game.mouseX = xpos; game.mouseY = ypos;
	}
}

void GameObject::Callbacks::WindowMoveCallback(int x, int y) noexcept
{
	if (game.fullscreen) return;
	// Store window location for information purposes
	game.windowX = x;
	game.windowY = y;
}

void GameObject::Callbacks::CloseCallback() noexcept
{
	// Exit out of main game loop
	game.mainLoopActive = false;
}

void GameObject::Callbacks::TakeScreenshot() noexcept
{
	bool error = false;
	const char *screenshotfolder = "Screenshots";

	// Ensure screenshot folder is valid by either creating if non-existent or replacing invalid folder
	try {
		// Create if there is no folder
		if (!FileManager::DirectoryExists(screenshotfolder)) FileManager::CreatePath(screenshotfolder);
		// Ensure it is a folder, not a file
		else if (FileManager::FileExists(screenshotfolder)) {
			FileManager::DeletePath(screenshotfolder);
			FileManager::CreatePath(screenshotfolder);
		}
	} catch (const FileManager::FileError &err) {
		TextFormat::warn(err.what(), "File error");
		error = true;
	}
	
	const auto ScreenshotEnd = [&](const std::string &finalText) {
		// Update chat with results
		AddChatMessage(finalText);
		// Temporarily show chat text
		m_app->m_chatText->ResetTextTime();
		m_app->m_chatText->textType = TextRenderer::TextType::TemporaryShow;
	};

	static std::string failedText = "Failed to save screenshot";

	if (error) {
		ScreenshotEnd(failedText);
		return;
	}

	// Lodepng needs pixel array in vector - 3 per pixel for RGB values
	const int w = game.width, h = game.height;
	std::vector<unsigned char> pixels(static_cast<std::size_t>(3 * w * h));

	// Read pixels from screen to vector
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

	// Get time in milliseconds (avoid name conflicts when taking screenshots in the same second)
	const double currentSecs = glfwGetTime();
	const int milliseconds = static_cast<int>((currentSecs - floor(currentSecs)) * 1000.0f);

	// Get *thread-safe* formatted time values depending on the system
	const std::time_t timeSecs = time(0u);
	std::tm timeValues;

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
	const std::string filenamefmt = fmt::format("Screenshot {:%Y-%m-%d %H-%M-%S}-{:03}.png", timeValues, milliseconds);

	// Get full path of image to save to
	const std::string directoryfmt = fmt::format("{}/{}", screenshotfolder, filenamefmt);

	// Flip Y axis as OGL uses opposite Y coordinates (bottom-top instead of top-bottom)
	std::vector<unsigned char> flippedPixels(3u * w * h);
	for (int x = 0; x < w; ++x) {
		for (int y = 0; y < h; ++y) {
			for (int s = 0; s < 3; ++s) {
				flippedPixels[(x + y * w) * 3 + s] = pixels[(x + (h - 1 - y) * w) * 3 + s];
			}
		}
	}

	// Save to PNG file in directory constructed above (RGB colours, no alpha)
	const bool success = lodepng::encode(directoryfmt, flippedPixels, w, h, LodePNGColorType::LCT_RGB) == 0u;

	// Clear both vectors to save memory usage
	flippedPixels.clear();
	pixels.clear();

	// Create result text to be displayed
	if (success) ScreenshotEnd("Screenshot saved as " + filenamefmt);
	else ScreenshotEnd(failedText);
}

void GameObject::Callbacks::ToggleInventory() noexcept
{
	m_app->player.inventoryOpened = !m_app->player.inventoryOpened; // Determine if inventory elements should be rendered
	glfwSetInputMode(game.window, GLFW_CURSOR, m_app->player.inventoryOpened ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED); // Free cursor if inventory is shown
	glfwSetCursorPos(game.window, static_cast<double>(game.width) / 2.0, static_cast<double>(game.height) / 2.0); // Reset mouse position

	game.focusChanged = true;
	game.showGUI = true;
}

void GameObject::Callbacks::ToggleFullscreen() noexcept
{
	game.focusChanged = true;

	if (!revBool(game.fullscreen)) {
		glfwSetWindowMonitor(game.window, NULL, game.windowX, game.windowY, game.fwidth, game.fheight, 0);
		return;
	}

	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode *mode = glfwGetVideoMode(monitor);
	glfwSetWindowMonitor(game.window, monitor, 0, 0, mode->width, mode->height, 0);
}

void GameObject::Callbacks::ApplyInput(int key, int action) noexcept
{
	// Select an inventory slot if user presses a number ('0' corresponds to the last slot)
	if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
		m_app->playerFunctions.UpdateScroll(key - GLFW_KEY_1);
		return;
	} else if (key == GLFW_KEY_0) {
		m_app->playerFunctions.UpdateScroll(9);
		return;
	}
	
	// Input type
	const int noCtrlInput = 1 << 3, noAltInput = 1 << 4, noShiftInput = 1 << 5;
	const int repeatableInput = GLFW_PRESS | GLFW_REPEAT;
	const int pressInput = GLFW_PRESS;

	struct InputData {
		int key, inputType;
		std::function<void()> action;
	};

	// Get limit/clamp values
	using namespace ValueLimits;

	// Functions for 'value' inputs
	const auto ChangeRender = [&](int change) {
		m_app->world.UpdateRenderDistance(m_app->world.chunkRenderDistance + static_cast<std::int32_t>(change));
	};
	const auto ChangeFOV = [&](double change) {
		ClampAdd(m_app->player.fov, change, fovLimit);
		m_app->playerFunctions.UpdateFrustum();
		m_app->player.moved = true;
	};
	
	static const InputData gameKeyFunctions[] = {
	// Default controls
	{ GLFW_KEY_SLASH, pressInput, [&]() { game.isChatCommand = true; BeginChat(); }},
	{ GLFW_KEY_T, pressInput, [&]() { game.isChatCommand = false; game.ignoreChatStart = true; BeginChat(); }},
	{ GLFW_KEY_ESCAPE, pressInput, [&]() { game.mainLoopActive = false; }},
	
	// Toggle inputs
	{ GLFW_KEY_X, pressInput, [&]() { glfwSwapInterval(revBool(game.vsyncFPS)); }},
	{ GLFW_KEY_E, pressInput, [&]() { ToggleInventory(); }},
	{ GLFW_KEY_F, pressInput, [&]() { revBool(game.hideFog); }},
	{ GLFW_KEY_C, pressInput, [&]() { revBool(m_app->player.doGravity); }},
	{ GLFW_KEY_N, pressInput, [&]() { revBool(m_app->player.noclip); }},
	{ GLFW_KEY_V, pressInput, [&]() { revBool(game.noGeneration); }},

	// Value inputs
	// Render distance
	{ GLFW_KEY_LEFT_BRACKET, pressInput, [&]() { ChangeRender(1); }},
	{ GLFW_KEY_RIGHT_BRACKET, pressInput, [&]() { ChangeRender(-1); }},
	// Speed
	{ GLFW_KEY_COMMA, repeatableInput, [&]() { m_app->player.defaultSpeed += 1.0f; }},
	{ GLFW_KEY_PERIOD, repeatableInput, [&]() { m_app->player.defaultSpeed -= 1.0f; }},
	// FOV
	{ GLFW_KEY_I, repeatableInput, [&]() { ChangeFOV(glm::radians(1.0)); }},
	{ GLFW_KEY_O, repeatableInput, [&]() { ChangeFOV(glm::radians(-1.0)); }},

	// Debug inputs
	{ GLFW_KEY_Z, pressInput, [&]() { glPolygonMode(GL_FRONT_AND_BACK, revBool(game.wireframe) ? GL_LINE : GL_FILL); }},
	{ GLFW_KEY_R, pressInput, [&]() {
		shader.InitShaders();
		m_app->world.textRenderer.UpdateShaderUniform();
		m_app->world.DebugChunkBorders(false);
		TextFormat::log("Reloaded shaders");
	}},
	{ GLFW_KEY_J, pressInput, [&]() { revBool(game.chunkBorders); m_app->world.DebugChunkBorders(false); }},
	{ GLFW_KEY_U, pressInput, [&]() { revBool(game.testbool); m_app->world.DebugReset(); }},

	// Function inputs
	{ GLFW_KEY_F1, pressInput, [&]() { revBool(game.showGUI); }},
	{ GLFW_KEY_F2, pressInput, [&]() { TakeScreenshot(); }},
	{ GLFW_KEY_F3, pressInput, [&]() { glfwSetInputMode(game.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); game.focusChanged = true; }},
	{ GLFW_KEY_F4, pressInput, [&]() { revBool(game.debugText); }},
	{ GLFW_KEY_F11, pressInput, [&]() { ToggleFullscreen(); }},
	};

	// Add to input information list
	if (action == GLFW_RELEASE) game.keyboardState.erase(key);
	else game.keyboardState.insert({ key, action });
	game.anyKeyPressed = game.keyboardState.size() > 0;

	// Determine modifier key status
	switch (key)
	{
		case GLFW_KEY_LEFT_CONTROL:
		case GLFW_KEY_RIGHT_CONTROL:
			game.holdingCtrl = action != GLFW_RELEASE;
			return;
		case GLFW_KEY_LEFT_ALT:
		case GLFW_KEY_RIGHT_ALT:
			game.holdingAlt = action != GLFW_RELEASE;
			return;
		case GLFW_KEY_LEFT_SHIFT:
		case GLFW_KEY_RIGHT_SHIFT:
			game.holdingShift = action != GLFW_RELEASE;
			return;
	}

	// Check if any input struct uses the pressed key
	const std::size_t keyFuncsSize = Math::size(gameKeyFunctions);
	const InputData *found = std::find_if(gameKeyFunctions, gameKeyFunctions + keyFuncsSize, [&](const InputData &id) { return id.key == key; });

	if (found != gameKeyFunctions + keyFuncsSize) {
		// Check if input action matches (or disallows certain modifier) and run the associated function if it is
		if ((found->inputType & noCtrlInput) && game.holdingCtrl) return;
		if ((found->inputType & noAltInput) && game.holdingAlt) return;
		if ((found->inputType & noShiftInput) && game.holdingShift) return;
		if (action & found->inputType) found->action();
	}
}

void GameObject::Callbacks::AddChatMessage(std::string message) noexcept
{
	if (!message.size()) { TextFormat::log("Empty chat message"); return; }

	// Format text for chat
	m_app->world.textRenderer.FormatChatText(message);

	// Append formatted text to current chat - assume previous text is formatted already
	std::string chatText = m_app->m_chatText->GetText();
	if (chatText.size()) chatText += '\n';
	chatText += message;

	// Remove previous lines if the whole text has too many lines
	const int linesToRemove = TextFormat::numChar(chatText, '\n') - m_app->world.textRenderer.maxChatLines;
	if (linesToRemove > 0) chatText = chatText.substr(TextFormat::findNthOf(chatText, '\n', linesToRemove) + static_cast<std::size_t>(1u));

	// Update text with new message
	m_app->world.textRenderer.ChangeText(m_app->m_chatText, chatText);
}

void GameObject::Callbacks::BeginChat() noexcept
{
	game.chatting = true;
	game.showGUI = true;
	m_app->player.inventoryOpened = false;

	// Make command and chat log text visible (if chat log has text)
	m_app->m_commandText->textType = TextRenderer::TextType::Default;
	m_app->m_chatText->textType = m_app->m_chatText->GetText().size() ? TextRenderer::TextType::Default : TextRenderer::TextType::Hidden;
}

void GameObject::Callbacks::AddCommandText(std::string newText) noexcept
{
	m_app->world.textRenderer.ChangeText(
		m_app->m_commandText, 
		m_app->m_commandText->GetText() + newText
	);
}

void GameObject::Callbacks::ApplyCommand()
{	
	// Command struct
	struct Command {
		typedef std::vector<int> intvec;
		Command(
			const std::string &name,
			const std::string &argsList,
			const std::string &description,
			const std::function<void()> &function,
			const std::function<void()> &qFunc = nullptr,
			const intvec givenNumArgs = { -2 }
		) noexcept : name(name), description(description), argsListText(argsList), function(function), queryFunc(qFunc) {
			// Determine number of arguments from arguments list:
			// Empty with query function: 1 argument
			// Empty without query function: -1 arguments (no argument checks)
			// All normal (e.g. 'x y z'): Number of spaces + 1 -> 2 + 1 = 3
			// Normal + optional (e.g. 'a b c *d *e *f'): Same as above but also includes another with optional argument(s) included -> 3, 4, 5, 6

			// If the 'numArgs' argument is not the default (-2), use the given value instead
			if (givenNumArgs.size() && givenNumArgs[0] != -2) {
				numArgs = givenNumArgs;
				return;
			}

			bool isEmpty = !argsList.size();
			if (isEmpty) numArgs = { queryFunc ? 1 : -1 };
			else {
				const int argcount = TextFormat::numChar(argsList, ' ') + 1, optionals = TextFormat::numChar(argsList, '*');
				if (optionals) for (int i = argcount - optionals; i <= argcount; ++i) numArgs.emplace_back(i);
				else numArgs = intvec{ argcount };
			}
		}

		const std::string name, description, argsListText;
		
		const std::function<void()> function;
		const std::function<void()> queryFunc;

		std::vector<int> numArgs;
	};

	static const std::string defQueryText = "*value";

	const auto DisplayHelpCommand = [&](const Command* commands, std::size_t size, int page) {
		// Page 1 is the first page - advance through help text depending on text lines limit and page
		static const std::string infoTxt =
			"A command starts with the character '/' and is formatted as such: '/name arg1 arg2'...\n"
			"If an argument has an asterik next to it, it is optional: '/name arg1 *arg2'.\n"
			"Entering some commands without any arguments will return the current value(s): "
			"'/time' returns the current in-game time in seconds. "
			"A tilde (~) in some arguments is treated as the current value: /tp 0 ~ 0 -> the '~' is replaced with your Y position. "
			"Numbers and negation can be included: /tp 0 -~5 0 -> tilde expression replaced with -(Y position + 5).\n";

		std::string finalText;
		page = glm::max(--page, 0); // Page 1 is the start, decrement for zero-indexing

		// Add descriptions and arguments of all commands
		while (size--) {
			const Command *cmd = &commands[size];
			if (cmd->description[0] == '_') continue; // Ignore debug commands
			const char ending = size ? '\n' : '\0';
			const std::string &argsText = cmd->argsListText.size() ? cmd->argsListText : (cmd->queryFunc == nullptr ? "(None)" : defQueryText);
			finalText += fmt::format("/{} {} Arguments: {}{}", cmd->name, cmd->description, argsText, ending);
		}
		
		// Determine section to show depending on the page using chat-formatted version of the help text
		m_app->world.textRenderer.FormatChatText(finalText);
		const std::size_t start = TextFormat::findNthOf(finalText, '\n', page * m_app->world.textRenderer.maxChatLines);
		const std::size_t end = TextFormat::findNthOf(finalText, '\n', (page + 1) * m_app->world.textRenderer.maxChatLines);
		finalText = finalText.substr(start, end - start);

		// Replace possible trailing new line, adding an ellipsis if there is still more info
		if (finalText.back() == '\n') finalText.pop_back();
		if (end != std::string::npos) finalText = finalText.substr(std::size_t{}, finalText.find_last_of(" ")) + "...";
		
		AddChatMessage(finalText); // Add help message to chat
	};

	// Shortcut variables
	World &world = m_app->world;
	WorldPlayer &plr = m_app->player;
	const glm::dvec3 &plrpos = plr.position;
	const WorldPosition plrposInt = ChunkValues::ToWorld(plrpos);
	Player &plrFuncs = m_app->playerFunctions;

	// Determine if a query should display a message
	queryChat = true;
	bool noQueryConversion = false;

	Command* allCommands;
	std::size_t commandsSize;
	
	using namespace ValueLimits;

	static Command commandsList[] = {
	// Function commands

	{ "tp", "x y z *yaw *pitch", "Teleports you to the specified x, y, z position. Optionally also sets the camera's direction", [&]() {
		CMDConv({ tcv(0, plrpos.x), tcv(1, plrpos.y), tcv(2, plrpos.z), tcv(3, plr.yaw), tcv(4, plr.pitch) });
		const glm::dvec3 pos = { DblArg(0), DblArg(1), DblArg(2) };
		plrFuncs.SetPosition(pos);
		if (HasArgument(3)) {
			plr.yaw = DblArg(3);
			if (HasArgument(4)) plr.pitch = DblArg(4);
			plrFuncs.UpdateCameraDirection();
		}
	}, [&]() {
		queryMult("position is", plrpos);
		queryMult("camera direction is", glm::dvec2(plr.yaw, plr.pitch), 3);
		noQueryConversion = true;
	}},
	{ "fill", "xFrom yFrom zFrom xTo yTo zTo block", "Fills from the 'from' position to the 'to' position with the specified block ID", [&]() {
		CMDConv({ tcv(0, plrposInt.x), tcv(1, plrposInt.y), tcv(2, plrposInt.z), tcv(3, plrposInt.x), tcv(4, plrposInt.y), tcv(5, plrposInt.z) });
		const auto IArg = [&](int i){ return IntArg<PosType>(i); };
		const std::uintmax_t changed = world.FillBlocks({IArg(0), IArg(1), IArg(2)}, { IArg(3), IArg(4), IArg(5) }, IntArg<ObjectIDTypeof, ObjectID>(6));
		if (changed) AddChatMessage(fmt::format("Too many blocks to change ({} > 32768). Try filling a smaller area.", changed));
	}},
	{ "set", "x y z block", "Sets the given position to the given block.", [&]() {
		world.SetBlock({ IntArg<PosType>(0), IntArg<PosType>(1), IntArg<PosType>(2) }, IntArg<ObjectIDTypeof, ObjectID>(3), true);
	}},
	{ "dcmp", "id", "_Creates a new file on the same directory with the ASM code for a given shader program ID", [&]() {
		std::vector<char> binary(65535);
		GLenum format{}; GLint length{};
		glGetProgramBinary(IntArg<GLsizei>(0), static_cast<GLsizei>(65535), &length, &format, binary.data());
		std::ofstream("shaderbinary.txt").write(binary.data(), length); 
	}, [&]() {
		noQueryConversion = true;
		if (!queryChat) return;

		std::string result = "Programs: ";
		shader.EachProgram([&](Shader::Program &prog) { result += fmt::format("'{}': {}, ", prog.name, prog.program); });
		AddChatMessage(result.substr(std::size_t{}, result.size() - static_cast<std::size_t>(2u)));
	}},
	{ "test", "*x *y *z *w", "_Sets 4 values for run-time testing", [&]() {
		for (int i=0;i<4;++i) if (HasArgument(i)) game.testvals[i] = DblArg(i); 
	}, [&]() { queryMult("debug values are", game.testvals); }},

	// Set/query commands

	{ "time",  "", "Changes the in-game time (in seconds)",
		[&]() { m_app->EditTime(true, DblArg(0)); }, [&]() { query("time", m_app->EditTime(false)); }
	},
	{ "speed", "", "Changes movement speed",
		[&]() { plr.defaultSpeed = DblArg(0); }, [&]() { query("speed", plr.currentSpeed); }
	},
	{ "tick", "", fmt::format("Changes the tick speed (how fast natural events occur) [{}, {}]", tickLimit.min, tickLimit.max),
		[&]() { game.tickSpeed = DblArg(0, -100.0, 100.0); }, [&]() { query("tick", game.tickSpeed); }
	},
	{ "rd", "", fmt::format("Changes the world's render distance [{}, {}]", renderLimit.min, renderLimit.max),
		[&]() { world.UpdateRenderDistance(IntArg<int>(0, 4, 200)); }, [&]() { query("render distance", m_app->world.chunkRenderDistance); }
	},
	{ "fov", "", fmt::format("Sets the camera FOV. [{}, {}]", fovLimit.min, fovLimit.max),
		[&]() { plr.fov = glm::radians(DblArg(0, 5.0, 160.0)); }, [&]() { query("FOV", glm::degrees(plr.fov)); }
	},

	// No argument commands

	{ "clear", "", "Clears the chat.", [&]() { world.textRenderer.ChangeText(m_app->m_chatText, ""); }},
	{ "trytxt", "", "_Used to test the text renderer.", [&]() { std::string msg; for (char x = '!'; x < 127; ++x) { msg += x; } AddChatMessage(msg); }},
	{ "exit", "", "Exits the game.", [&]() { game.mainLoopActive = false; }},
	{ "help", "*page", "Prints out some helpful text depending on the given page.",
		[&]() { DisplayHelpCommand(allCommands, commandsSize, HasArgument(0) ? IntArg<int>(0) : 0); }
	},
	{ "none", "", "_", [&]() {}}
	};

	allCommands = commandsList;
	commandsSize = Math::size(commandsList);
	Command* found;

	{
		// Check if there is any text in the first place
		std::string chatCommand = m_app->m_commandText->GetText();
		if (!chatCommand.size()) return;
	
		// Add current chat input to history - 50 max for now
		m_previousChats.insert(m_previousChats.begin(), chatCommand);
		if (m_previousChats.size() > 50u) m_previousChats.pop_back();
	
		// Determine if it is a chat message or command
		if (chatCommand[0] != '/') {
			AddChatMessage("[YOU] " + chatCommand);
			return;
		}
		
		const std::size_t one = static_cast<std::size_t>(1u);
		const std::string cmd = chatCommand.substr(one, chatCommand.find(' ') - one); // Get command name
		const std::size_t startIndex = chatCommand.find(' ');

		// Remove 'name' section and trailing spaces
		chatCommand = chatCommand.substr((startIndex == std::string::npos ? cmd.size() : startIndex) + one, chatCommand.find_last_not_of(' ')); 

		// Search command list to see if the given command name matches
		found = std::find_if(commandsList, commandsList + commandsSize, [&](const Command &c) { return c.name == cmd; });
		if (found == commandsList + commandsSize) {
			AddChatMessage(fmt::format("/{} is not a valid command. Type /help for help.", cmd)); // Warn for invalid commands
			return;
		}

		CMD_args.clear(); // Clear arguments list

		// Get each argument from the command given using spaces
		if (chatCommand.size()) TextFormat::strsplit(CMD_args, chatCommand, ' ');
		
		// Remove any invalid/empty arguments
		CMD_args.erase(std::remove_if(CMD_args.begin(), CMD_args.end(), [&](const std::string &v) { return v.find_first_not_of(' ') == std::string::npos; }), CMD_args.end());
	}

	const int numCmdArgs = static_cast<int>(CMD_args.size());

	// Attempt to execute command
	try {
		// Check for value query
		bool isHelp = numCmdArgs == 1 && CMD_args[0] == "?";
		bool hasQuery = found->queryFunc != nullptr;
		bool isQuery = !isHelp && hasQuery;

		if (isQuery && numCmdArgs == 0) { found->queryFunc(); return; } // Print out current value(s) -> queryChat = true

		// Check for correct arguments amount (-1 means ignore check)
		if (isHelp || (found->numArgs[0] != -1 && std::find(found->numArgs.begin(), found->numArgs.end(), numCmdArgs) == found->numArgs.end())) {
			AddChatMessage(fmt::format("Usage: /{} {}", found->name, hasQuery && !found->argsListText.size() ? defQueryText : found->argsListText));
			return;
		}
		
		queryChat = false; // Using query to convert rather than display value(s)
		if (hasQuery) found->queryFunc();
		if (hasQuery && !noQueryConversion) {
			// Use query string to convert specific arguments
			// Example: { i, "12.3", true } is returned, only one conversion is made, using 'i' as the starting argument index.
			// If { 2, "12.3 64.5 7.2", true } is returned, 3 conversions are done (3 numbers) starting from the 2nd argument index.
			int i = cvd.index;
			std::vector<std::string> eachConv;
			TextFormat::strsplit(eachConv, cvd.strarg, ' ');
			for (const std::string &str : eachConv) CMDConv({ i++, cvd.decimal, str });
		}

		found->function();
	}
	// Warn about found invalid argument if any
	catch (const std::invalid_argument&) { AddChatMessage(fmt::format("Unexpected '{}' at command argument {}", GetArg(currentCMDInd), currentCMDInd + 1)); }
	// Warn about converting extremely large numbers
	catch (const std::out_of_range&) { AddChatMessage(fmt::format("Argument {} ({:.5}...) is too large", currentCMDInd + 1, GetArg(currentCMDInd))); }
}

double GameObject::Callbacks::DblArg(int index, double min, double max) { return glm::clamp(std::stod(GetArg(index)), min, max); }
void GameObject::Callbacks::CMDConv(const std::vector<ConversionData> &argsConversions) { for (const auto &val : argsConversions) CMDConv(val); }

void GameObject::Callbacks::CMDConv(const ConversionData &data)
{
	if (!HasArgument(data.index)) return; // Check if argument is in range
	
	std::string &arg = GetArg(data.index); // Get the command argument
	const std::string &actual = data.strarg; // Get the current value as a string
	if (!actual.size()) return;

	const std::size_t one = static_cast<std::size_t>(1u);
	if (TextFormat::numChar(arg, '~') != 1) return; // Convert if only 1 tilde is found
	const std::size_t tildeIndex = arg.find('~'); // Index of tilde
	if (tildeIndex > one) throw std::invalid_argument(""); // A tilde index of >1 is invalid

	std::string afterTildeNumber = arg.substr(tildeIndex + one); // Text after tilde character
	if (!afterTildeNumber.size()) afterTildeNumber = "0"; // Default to "0" if empty
	const char beforeTildeVal = tildeIndex ? arg[0] : '\0'; // Character before tilde character

	// Check for a possible negative symbol before the tilde or for empty text
	const bool isNegative = beforeTildeVal == '-';
	if (!isNegative && static_cast<int>(beforeTildeVal)) throw std::invalid_argument(""); // No other non-zero character before the tilde is allowed

	// Combine all parts of the argument into one number (add number after tilde, use negation if found)
	if (data.decimal) arg = fmt::to_string((std::stod(actual) + std::stod(afterTildeNumber)) * (isNegative ? -1.0 : 1.0));
	else arg = fmt::to_string(TextFormat::strtoimax(actual) + TextFormat::strtoimax(afterTildeNumber) * static_cast<std::intmax_t>(isNegative ? -1 : 1));
}
