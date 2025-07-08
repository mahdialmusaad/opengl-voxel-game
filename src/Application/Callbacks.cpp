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

		const auto ExitChat = [&](TextRenderer::TextType chatType = TextRenderer::TextType::Hidden) {
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
			} else ExitChat();
		}
		// Exit chat if escape is pressed - hide chat instantly
		else if (key == GLFW_KEY_ESCAPE) ExitChat();
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
	if (width <= 0 || height <= 0) {
		game.minimized = true;
		glfwSwapInterval(10); // Update every 10 screen frames
		return;
	}
	else if (game.minimized) {
		game.minimized = false;
		// Set to default game frame update state
		glfwSwapInterval(game.maxFPS ? 0 : 1);
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

	// The resulting text to be displayed on screen
	std::string newScreenshotText = "Failed to save screenshot";

	if (!error) {
		// Lodepng needs pixel array in vector - 3 per pixel for RGB values
		const int w = game.width, h = game.height;
		std::vector<unsigned char> pixels(static_cast<std::size_t>(3 * w * h));

		// Read pixels from screen to vector
		glReadBuffer(GL_FRONT);
		glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

		// Get time in milliseconds (avoid name conflicts when taking screenshots in the same second)
		const double currentSecs = glfwGetTime();
		const int milliseconds = static_cast<int>((currentSecs - floor(currentSecs)) * 1000.0f);

		// Get *thread-safe* formatted time values
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
		std::vector<unsigned char> newHalf(3u * w * h);
		for (int x = 0; x < w; ++x) {
			for (int y = 0; y < h; ++y) {
				for (int s = 0; s < 3; ++s) {
					newHalf[(x + y * w) * 3 + s] = pixels[(x + (h - 1 - y) * w) * 3 + s];
				}
			}
		}

		// Save to PNG file in directory constructed above (RGB colours, no alpha)
		const bool success = lodepng::encode(directoryfmt, newHalf, w, h, LodePNGColorType::LCT_RGB) == 0u;

		newHalf.clear();
		pixels.clear();

		// Create result text to be displayed
		if (success) newScreenshotText = "Screenshot saved as " + filenamefmt;
	}

	
	AddChatMessage(newScreenshotText); // Update chat with results

	// Temporarily show chat text
	m_app->m_chatText->ResetTextTime();
	m_app->m_chatText->textType = TextRenderer::TextType::TemporaryShow;
}

void GameObject::Callbacks::ToggleInventory() noexcept
{
	m_app->player.inventoryOpened = !m_app->player.inventoryOpened; // Determine if inventory elements should be rendered
	glfwSetInputMode(game.window, GLFW_CURSOR, m_app->player.inventoryOpened ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED); // Free cursor if inventory is shown
	glfwSetCursorPos(game.window, static_cast<double>(game.width) / 2.0, static_cast<double>(game.height) / 2.0); // Reset mouse position

	game.focusChanged = true;
	game.showGUI = true;
}

void GameObject::Callbacks::ApplyInput(int key, int action) noexcept
{
	// Pressing numbers correlate with slot
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

	// Flip boolean
	const auto revBool = [&](bool &b) -> bool { b = !b; return b; };

	typedef std::pair<int, std::function<void()>> inputPair;
	static const std::unordered_map<int, inputPair> keyFunctionMap = {
		// Single-press inputs

		{ GLFW_KEY_ESCAPE, { pressInput, [&]() {
			game.mainLoopActive = false;
		}}},
		{ GLFW_KEY_F, { pressInput, [&]() {
			if (revBool(game.fullscreen)) {
				GLFWmonitor *monitor = glfwGetPrimaryMonitor();
				const GLFWvidmode *mode = glfwGetVideoMode(monitor);
				glfwSetWindowMonitor(game.window, monitor, 0, 0, mode->width, mode->height, 0);
			} else glfwSetWindowMonitor(game.window, NULL, game.windowX, game.windowY, game.fwidth, game.fheight, 0);
			game.focusChanged = true;
		}}},
		{ GLFW_KEY_E, { pressInput, [&]() {
			ToggleInventory();
		}}},
		{ GLFW_KEY_Z, { pressInput, [&]() {
			glPolygonMode(GL_FRONT_AND_BACK, revBool(game.wireframe) ? GL_LINE : GL_FILL);
		}}},
		{ GLFW_KEY_X, { pressInput, [&]() {
			glfwSwapInterval(!revBool(game.maxFPS));
		}}},
		{ GLFW_KEY_C, { pressInput, [&]() {
			revBool(m_app->player.doGravity);
		}}},
		{ GLFW_KEY_N, { pressInput, [&]() {
			revBool(m_app->player.noclip);
		}}},
		{ GLFW_KEY_V, { pressInput, [&]() {
			revBool(game.noGeneration);
		}}},
		{ GLFW_KEY_R, { pressInput, [&]() {
			game.shader.InitShader();
			m_app->world.textRenderer.UpdateShaderUniform();
			m_app->UpdateAspect();
		}}},
		{ GLFW_KEY_U, { pressInput, [&]() {
			revBool(game.testbool);
			m_app->world.DebugReset();
		}}},
		{ GLFW_KEY_T, { pressInput, [&]() {
			game.isChatCommand = false;
			game.ignoreChatStart = true;
			BeginChat();
		}}},
		{ GLFW_KEY_J, { pressInput, [&]() {
			revBool(game.chunkBorders);
			m_app->world.DebugChunkBorders(false);
		}}},
		{ GLFW_KEY_F1, { pressInput, [&]() {
			revBool(game.showGUI);
		}}},
		{ GLFW_KEY_F2, { pressInput, [&]() {
			TakeScreenshot();
		}}},
		{ GLFW_KEY_F3, { pressInput, [&]() {
			glfwSetInputMode(game.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			game.focusChanged = true;
		}}},
		{ GLFW_KEY_F4, { pressInput, [&]() {
			revBool(game.debugText);
		}}},
		{ GLFW_KEY_SLASH, { pressInput, [&]() {
			game.isChatCommand = true;
			BeginChat();
		}}},
		{ GLFW_KEY_LEFT_BRACKET, { pressInput, [&]() {
			m_app->world.UpdateRenderDistance(m_app->world.chunkRenderDistance + 1);
		}}},
		{ GLFW_KEY_RIGHT_BRACKET, { pressInput, [&]() {
			m_app->world.UpdateRenderDistance(m_app->world.chunkRenderDistance - 1);
		}}},

		// Repeatable inputs

		{ GLFW_KEY_COMMA, { repeatableInput, [&]() {
			m_app->player.defaultSpeed += 1.0f;
		}}},
		{ GLFW_KEY_PERIOD, { repeatableInput, [&]() {
			m_app->player.defaultSpeed -= 1.0f;
		}}},
		{ GLFW_KEY_I, { repeatableInput, [&]() {
			m_app->player.fov += glm::radians(1.0f);
			m_app->UpdatePerspective();
		}}},
		{ GLFW_KEY_O, { repeatableInput, [&]() {
			m_app->player.fov -= glm::radians(1.0f);
			m_app->UpdatePerspective();
		}}}
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

	// Check if key is listed in input map
	const auto &keyboardPair = keyFunctionMap.find(key);

	if (keyboardPair != keyFunctionMap.end()) {
		const int inputAction = keyboardPair->second.first;
		// Check if input action matches (or disallows certain modifier) and run the associated function if it is
		if ((inputAction & noCtrlInput) && game.holdingCtrl) return;
		if ((inputAction & noAltInput) && game.holdingAlt) return;
		if ((inputAction & noShiftInput) && game.holdingShift) return;
		if (action & inputAction) keyboardPair->second.second();
	}
}

void GameObject::Callbacks::AddChatMessage(std::string message) noexcept
{
	if (!message.size()) { TextFormat::log("Empty chat message"); return; }
	
	// Adding on to previous chat log - add new line if there is existing text
	std::string newText = m_app->m_chatText->GetText();
	if (m_app->m_chatText->GetText().size()) newText += '\n';

	// Get list of words - characters separated by a space
	std::vector<std::string> words;
	{
		std::string currentWord;
		for (const char &c : message) { if (c == ' ') { words.emplace_back(currentWord + c); currentWord.clear(); } else currentWord += c; }
		words.emplace_back(currentWord + ' ');
	}

	// Add a new line when going over character limit, either between words 
	// or in the middle of a word that is larger than the character limit
	int totalChars = 0;
	bool isStart = true;
	for (std::size_t i{}; i < words.size(); ++i) {
		std::string &word = words[i];
		totalChars += static_cast<int>(word.size());
		if (totalChars > m_app->world.textRenderer.maxChatCharacters) { 
			if (isStart) word.insert(word.begin() + m_app->world.textRenderer.maxChatCharacters, '\n');
			else {
				std::string &prevWord = words[i-1];
				prevWord[static_cast<int>(prevWord.size()) - 1] = '\n'; 
			}
			totalChars = 0;
			isStart = true;
		}
		isStart = false;
	}

	for (const std::string &word : words) newText += word; // Add all words to end of chat

	// Remove previous lines if the whole text is too large
	int linesToRemove = static_cast<int>(std::count(newText.begin(), newText.end(), '\n')) - m_app->world.textRenderer.maxChatLines;
	if (linesToRemove > 0) {
		std::size_t newIndex{};
		while (linesToRemove--) { newIndex = newText.find_first_of('\n', newIndex) + static_cast<std::size_t>(1); }
		newText = newText.substr(newIndex);
	}

	// Update text with new message
	m_app->world.textRenderer.ChangeText(m_app->m_chatText, newText);
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
		Command(
			std::string name,
			std::string argsList,
			std::string description,
			std::function<void()> function,
			std::function<std::string()> qFunc = nullptr,
			std::vector<int> numArgs = { -2 }
		) noexcept : name(name), description(description), argsListText(argsList), function(function), queryFunc(qFunc) {
			// Determine number of arguments from arguments list:
			// Empty with query function: 1 argument
			// Empty without query function: -1 arguments (no argument checks)
			// All normal (e.g. 'x y z'): Number of spaces + 1 -> 3
			// Normal + optional (e.g. 'a b c *d *e *f'): Same as above but also includes another with optional argument(s) included -> 3, 6

			// If the 'numArgs' argument is not the default (-2), use the given value instead
			if (numArgs.size() && numArgs[0] != -2) {
				allowedNumArgs = numArgs;
				return;
			}

			bool isEmpty = !argsList.size();
			if (isEmpty) { allowedNumArgs = { queryFunc ? 1 : -1 }; customConvert = allowedNumArgs[0] == 1; }
			else {
				const int numSpaces = static_cast<int>(std::count(argsList.begin(), argsList.end(), ' ')); // 5
				const int numOptionals = static_cast<int>(std::count(argsList.begin(), argsList.end(), '*')); // 3
				if (numOptionals) allowedNumArgs = { numSpaces - numOptionals + 1, numSpaces + 1 };
				else allowedNumArgs = { numSpaces + 1 };
			}
		}

		std::string name, description, argsListText;
		std::function<void()> function;
		std::function<std::string()> queryFunc;
		std::vector<int> allowedNumArgs;
		bool customConvert = false;
	};

	static const std::string queryText = "*value";

	// TODO: Help command 'pages'?
	const auto GetHelpCommand = [&](const Command* commands, std::size_t size) -> std::string {
		std::string txt = "A command starts with the character '/'. If an argument has an asterik next to it, it is optional. Using a tilde ('~') with some arguments will be replaced with the actual value (e.g. /tp 0 ~ 0) will not change your Y position.";
		while (size--) {
			const Command *cmd = &commands[size];
			txt += fmt::format("/{} {} Arguments: {}{}", cmd->name, cmd->description, cmd->argsListText.size() ? cmd->argsListText : queryText, size > 0 ? "\n" : "");
		}
		return txt;
	};

	#define tstr(x) fmt::to_string(x)

	World &world = m_app->world;
	PlayerObject &plr = m_app->player;
	const glm::dvec3 plrpos = plr.position;
	Player &plrFuncs = m_app->playerFunctions;

	bool queryChat = true;
	const auto query = [&](std::string name, double val) -> std::string {
		if (queryChat) AddChatMessage(fmt::format("Current {} is {:.3f}", name, val));
		return fmt::to_string(val);
	}; // Add to chat with current value, return it for tilde conversion

	Command* allCommands;
	std::size_t commandsSize;

	// List of all arguments
	static Command gameCommandsMap[] = {
		// Function commands

		{ "tp", "x y z *yaw *pitch", "Teleports you to the specified x, y, z position. Optionally also sets the camera's direction.", [&]() {
			DoConvert({{0,tstr(plrpos.x)},{1,tstr(plrpos.y)},{2,tstr(plrpos.z)},{3,tstr(plr.yaw)},{4,tstr(plr.pitch)}});
			const glm::dvec3 pos = { DblArg(0), DblArg(1), DblArg(2) };
			plrFuncs.SetPosition(pos);
			if (HasArgument(4)) { plr.yaw = DblArg(3); plr.pitch = DblArg(4); plrFuncs.UpdateCameraDirection(); }
		}},
		{ "fill", "xFrom yFrom zFrom xTo yTo zTo block", "Fills from the 'from' position to the 'to' position with the specified block ID.", [&]() {
			DoConvert({{0,tstr(plrpos.x)},{1,tstr(plrpos.y)},{2,tstr(plrpos.z)},{3,tstr(plrpos.x)},{4,tstr(plrpos.y)},{5,tstr(plrpos.z)}});
			const auto IArg = [&](int i){ return IntArg<PosType>(i); };
			fmt::println("{}: {} = {}, {}: {} = {}", 1, GetArgument(1), IArg(1), 4, GetArgument(4), IArg(4));
			if (!world.FillBlocks({ IArg(0), IArg(1), IArg(2) }, { IArg(3), IArg(4), IArg(5) }, IntArg<ObjectID>(6))) {
				AddChatMessage("Too many blocks to change. Try decreasing the number of blocks you are filling.");
			};
		}},
		{ "dcmp", "pid", "DEBUG: Creates a new file on the same directory with the ASM code for a given shader program ID.", [&]() {
			std::vector<char> binary(20 * 1024);
			GLenum format = 0; GLint length = 0;
			glGetProgramBinary(IntArg<GLsizei>(0), 20 * 1024, &length, &format, binary.data());
			std::ofstream("bin.txt").write(binary.data(), length); 
		}},
		{ "test", "*x *y *z *w", "DEBUG: Sets 4 values for run-time testing.", [&]() { for (int i=0;i<4;++i) if (HasArgument(i)) game.testvals[i] = DblArg(i); 
		}, [&]() { 
			const glm::vec4 v = game.testvals;
			static std::string fmtText = "{:.3f} {:.3f} {:.3f} {:.3f}";
			AddChatMessage(fmt::format("Current debug values are: {}", v.x, v.y, v.z, v.w, fmtText));
			return ""; // TODO: Multiple query conversions
		}},

		// Set/query commands

		{ "time",  "", "Changes the in-game time (in seconds).", [&]() { m_app->EditTime(true, DblArg(0)); }, [&]() { return query("time", m_app->EditTime(false)); }},
		{ "speed", "", "Changes movement speed.", [&]() { plr.defaultSpeed = DblArg(0); }, [&]() { return query("speed", plr.currentSpeed ); }},
		{ "tick",  "", "Changes the game tick speed (how fast natural events occur). [-100, 100]", [&]() { game.tickSpeed = DblArg(0, -100.0, 100.0); }, [&]() { return query("tick", game.tickSpeed); }},
		{ "rd",    "", "Changes the world's render distance. [0, 50]", [&]() { world.UpdateRenderDistance(IntArg<int>(0, 0, 50)); }, [&]() { return query("render distance", m_app->world.chunkRenderDistance); }},
		{ "fov",   "", "Sets the camera FOV.", [&]() { plr.fov = glm::radians(DblArg(0)); }, [&]() { return query("FOV", glm::degrees(plr.fov)); }},

		// No argument commands

		{ "clear", "", "Clears the chat.", [&]() { world.textRenderer.ChangeText(m_app->m_chatText, ""); }},
		{ "trytxt","", "DEBUG: Used to test the text renderer. ", [&]() { AddChatMessage("!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEF"); }},
		{ "exit",  "", "Exits the game.", [&]() { game.mainLoopActive = false; }},
		{ "help",  "", "Prints out this message.", [&]() { AddChatMessage(GetHelpCommand(allCommands, commandsSize)); }},
	};
	const std::size_t mapSize = Math::size(gameCommandsMap);

	allCommands = gameCommandsMap;
	commandsSize = Math::size(gameCommandsMap);

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

		chatCommand = chatCommand.substr(1, chatCommand.find_last_not_of(" ")); // Remove trailing spaces
		chatCommand += " "; // Add ending space for below to detect it
		
		// Search command list to see if the given name matches
		const auto CheckExists = [&](const std::string &cmd) {
			found = std::find_if(gameCommandsMap, gameCommandsMap + mapSize, [&](const Command &c) { return c.name == cmd; });
			if (found == gameCommandsMap + mapSize) {
				AddChatMessage(fmt::format("/{} is not a valid command. Type /help for help.", cmd)); // Warn for invalid commands
				return true;
			} else return false;
		};
		
		// Get each argument from the command given using spaces
		std::size_t last{};
		bool isFirst = true;
		
		do {
			std::size_t ind = chatCommand.find_first_of(" ", last);
			if (ind == std::string::npos) break;
			const std::string arg = chatCommand.substr(last, ind - last);
			if (isFirst && CheckExists(arg)) return; else isFirst = false;
			CMD_args.emplace_back(arg);
			last = ++ind;
		} while (true);
	}

	const std::string cmd = CMD_args[0]; // Get command name (first argument)
	CMD_args.erase(CMD_args.begin()); // Delete it so index 0 points to first argument
	const int numArgs = static_cast<int>(CMD_args.size());

	// Attempt to execute command
	try {
		// Check for value query
		bool isQuery = found->queryFunc != nullptr;
		if (isQuery && numArgs == 0) found->queryFunc();
		else {
			// Check for correct arguments amount (-1 means ignore check)
			if (found->allowedNumArgs[0] != -1 && std::find(found->allowedNumArgs.begin(), found->allowedNumArgs.end(), numArgs) == found->allowedNumArgs.end()) {
				AddChatMessage(fmt::format("Usage: /{} {}", found->name, isQuery ? queryText : found->argsListText));
			} else {
				// Custom conversions for certain commands
				if (found->customConvert) { queryChat = false; ConvertArgument(CMD_args[0], found->queryFunc()); }
				found->function();
			}
		}
	}
	// Warn about found invalid argument if any
	catch (const std::invalid_argument&) { AddChatMessage(fmt::format("Unexpected '{}' at command argument {}", GetArgument(CMD_currentIndex), CMD_currentIndex)); }
	// Catch error for converting extremely large numbers
	catch (const std::out_of_range&) { AddChatMessage(fmt::format("Argument {} ({:.5}...) is too large", CMD_currentIndex, GetArgument(CMD_currentIndex))); }

	CMD_args.clear(); // Clear arguments list for next time
}

void GameObject::Callbacks::ConvertArgument(std::string &arg, const std::string &actual)
{
	const std::size_t one = static_cast<std::size_t>(1);
	if (std::count(arg.begin(), arg.end(), '~') != static_cast<std::ptrdiff_t>(1)) return; // If no tilde is found, don't change anything
	const std::size_t tildeIndex = arg.find_first_of('~'); // Index of tilde
	if (tildeIndex > one) throw std::invalid_argument(""); // A tilde index of >1 is invalid

	const bool isDecimal = actual.find('.') != std::string::npos;
	std::string afterTildeNumber = arg.substr(tildeIndex + one); // Text after tilde character (default to 0)
	if (!afterTildeNumber.size()) afterTildeNumber = "0";
	const std::string beforeTildeVal = arg.substr(static_cast<std::size_t>(0), tildeIndex); // Text before tilde character

	bool isNegative = false;
	// Check for a possible sign modifer before the tilde (+ or -) or for empty text
	if (beforeTildeVal.find_first_of("+-") != std::string::npos) isNegative = beforeTildeVal[0] == '-';
	else if (beforeTildeVal.size()) throw std::invalid_argument(""); // No other character before the tilde is allowed

	// Combine all parts of the argument into one number (add number after tilde, use sign modifier before tilde)
	if (isDecimal) arg = fmt::to_string((std::stod(actual) + std::stod(afterTildeNumber)) * (isNegative ? -1.0 : 1.0));
	else arg = fmt::to_string((TextFormat::strtoi64(actual) + TextFormat::strtoi64(afterTildeNumber)) * static_cast<std::int64_t>(isNegative ? -1 : 1));
}

void GameObject::Callbacks::DoConvert(const ConversionList &argsConversions)
{
	for (const auto &val : argsConversions) {
		if (!HasArgument(val.first)) continue; // Check if argument is in range
		ConvertArgument(GetArgument(val.first), val.second); // Change argument to final value of conversion
	}
}
