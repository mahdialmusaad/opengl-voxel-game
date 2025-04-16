#include "Utility/Application.hpp"

Badcraft::Badcraft() :
	world(player),
	player(playerFunctions.player)
{
	TextFormat::log("Application enter");
	playerFunctions.world = &world;
	callbacks.app = this;

	// OGL rendering settings
	glEnable(GL_PROGRAM_POINT_SIZE); // Make GL_POINTS size set in shaders instead of having some global value
	glEnable(GL_DEPTH_TEST); // Enable depth testing so objects are rendered properly
	glDepthFunc(GL_LEQUAL); // Suitable depth testing function
	
	glEnable(GL_BLEND); // Transparency blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Suitable blend function

	glEnable(GL_CULL_FACE); // Enable face culling
	glCullFace(GL_BACK); // The inside of objects are not seen normally so they can be ignored

	glLineWidth(5.0f); // Make block raycast outline easier to see

	// Setup buffers for shared program data (update size to reflect shader ubo size)
	OGL::SetupUBO(m_matricesUBO, 0, sizeof(glm::mat4[3]));
	OGL::SetupUBO(m_timesUBO, 1, sizeof(float[5]));
	OGL::SetupUBO(m_coloursUBO, 2, sizeof(glm::vec4[3]));
	OGL::SetupUBO(m_positionsUBO, 3, sizeof(glm::dvec4[2]));
	OGL::SetupUBO(m_sizesUBO, 4, sizeof(float[7]));

	// Update block texture size in shader UBO
	float sizesData[] = {
		16.0f / static_cast<float>(game.blocksTextureInfo.width),
		1.0f / static_cast<float>(game.textTextureInfo.width)
	};
	OGL::UpdateUBO(m_sizesUBO, 0, sizeof(sizesData), sizesData);

	// Aspect ratio for GUI sizing (also updates perspective matrix)
	// ***	Must do this BEFORE text creation so the text objects use the 
	//		correct size multipliers without needing to be recalculated ***
	UpdateAspect();

	// Text object creation
	TextRenderer &tr = world.textRenderer;

	std::string formatText = std::format("{}\n{}\nSeed: {}", 
		glfwGetVersionString(), 
		reinterpret_cast<const char*>(glGetString(GL_RENDERER)), 
		game.noiseGenerators.continentalness.seed
	);
	tr.CreateText(glm::vec2(0.01f, 0.01f), formatText); // Combine static text for less draw calls and memory
	
	m_screenshotText = tr.CreateText(glm::vec2(0.18f, 0.8f), "", TextRenderer::T_Type::Hidden, 12u); // Screenshot info
	m_chatText = tr.CreateText(glm::vec2(0.01f, 0.85f), "", TextRenderer::T_Type::Hidden); // Chat text
	m_infoText = tr.CreateText(glm::vec2(0.01f, 0.18f), ""); // Reserve text object for dynamic info text
	m_debugText = tr.CreateText(glm::vec2(0.01f, 0.3f), ""); // Debugging purposes

	#ifndef BADCRAFT_1THREAD
	world.StartThreadChunkUpdate(); // Update chunks when needed on a seperate thread
	#endif

	// Clear all image data in game struct as they are no longer needed
	game.blocksTextureInfo.data.clear();
	game.textTextureInfo.data.clear();
	game.inventoryTextureInfo.data.clear();

	TextFormat::log("Application exit");
}

void Badcraft::Main()
{
	constexpr double windowUpdateInterval = 1.0f / 20.0f; // Information update frequency
	double windowTitleUpdateTime = 0.0; // The time since the last window title update
	double largestUpdateTime = 0.0; // The largest amount of time between two frames

	const auto CheckKey = [](int key, int action) noexcept {
		return glfwGetKey(game.window, key) == action;
	};

	int avgFPSCount = 0; // Number of frames since last title update
	
	// Main game loop
	TextFormat::log("Main loop enter");

	game.mainLoopActive = true;
	#ifdef BADCRAFT_1THREAD
	world.TestChunkUpdate();
	#endif

	while (game.mainLoopActive) {
		// Time values
		game.currentFrameTime = glfwGetTime();
		game.deltaTime = game.currentFrameTime - m_lastFrameTime;
		game.tickedDeltaTime = game.deltaTime * game.tickSpeed;
		if (game.deltaTime > largestUpdateTime) largestUpdateTime = game.deltaTime;
		game.tickedFrameTime += game.tickedDeltaTime;
		windowTitleUpdateTime += game.deltaTime;
		m_updateTime += game.deltaTime;
		m_lastFrameTime = game.currentFrameTime;
		avgFPSCount++;

		glfwPollEvents(); // Poll I/O such as key and mouse inputs at specified intervals
		
		if (game.anyKeyPressed && !game.chatting) {
			// Frame-dependent inputs (not toggle inputs)
			if (CheckKey(GLFW_KEY_W, GLFW_PRESS)) playerFunctions.ProcessKeyboard(WorldDirection::Front);
			if (CheckKey(GLFW_KEY_S, GLFW_PRESS)) playerFunctions.ProcessKeyboard(WorldDirection::Back);
			if (CheckKey(GLFW_KEY_A, GLFW_PRESS)) playerFunctions.ProcessKeyboard(WorldDirection::Left);
			if (CheckKey(GLFW_KEY_D, GLFW_PRESS)) playerFunctions.ProcessKeyboard(WorldDirection::Right);

			if (CheckKey(GLFW_KEY_LEFT_SHIFT, GLFW_PRESS)) player.currentSpeed = player.defaultSpeed * 2.0f;
			else if (CheckKey(GLFW_KEY_LEFT_CONTROL, GLFW_PRESS)) player.currentSpeed = player.defaultSpeed * 0.25f;
			if (CheckKey(GLFW_KEY_LEFT_SHIFT, GLFW_RELEASE) && CheckKey(GLFW_KEY_LEFT_CONTROL, GLFW_RELEASE)) player.currentSpeed = player.defaultSpeed;
		}

		// Apply smoothed movement using velocity and other position-related functions
		playerFunctions.UpdateMovement();

		if (player.moved) {
			player.moved = false;
			// Update view matrix for complicated 3D maths to determine screen position of vertices in shader
			playerFunctions.SetViewMatrix(m_matrices[View]);
			m_matrices[World] = m_matrices[Perspective] * m_matrices[View];

			// Update frustum values for frustum culling
			playerFunctions.UpdateFrustum();
			// Sort the world buffers to use new frustum values
			world.SortWorldBuffers();

			// Remove translation from skybox matrix so it always appears around the player
			m_matrices[Origin] = m_matrices[Perspective] * glm::mat4(glm::mat3(m_matrices[View]));

			// Update world matrix values in GPU shaders
			OGL::UpdateUBO(m_matricesUBO, 0, sizeof(glm::mat4[2]), m_matrices + 2);
		}

		// Update the world buffers to show any chunk updates (block placing, new chunks etc)
		if (world.threadUpdateBuffers) world.UpdateWorldBuffers();
		// Update shader UBO values (time, colours)
		UpdateFrameValues();

		// Game rendering
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear screen depth and colour buffer
		
		playerFunctions.RenderBlockOutline();
		world.DrawWorld();
		
		m_skybox.RenderSky();
		m_skybox.RenderStars();

		m_skybox.RenderClouds();

		if (showGUI) {
			glClear(GL_DEPTH_BUFFER_BIT); // Clear depth again to guarantee GUI draws on top
			world.textRenderer.RenderText();
			playerFunctions.RenderPlayerGUI();
		}

		// Window title update
		if (windowTitleUpdateTime >= windowUpdateInterval) {
			// Determine FPS values
			m_avgFPS = static_cast<int>(static_cast<double>(avgFPSCount) / windowTitleUpdateTime);
			m_lowFPS = static_cast<int>(1.0 / largestUpdateTime);
			m_nowFPS = static_cast<int>(1.0 / game.deltaTime);

			// Update title window with FPS information
			glfwSetWindowTitle(game.window, 
				std::format("badcraft - {} FPS | {} AVG | {} LOW", m_nowFPS, m_avgFPS, m_lowFPS).c_str()
			);

			// Update displayed game info text
			TextUpdate();

			// Reset values
			largestUpdateTime = game.deltaTime;
			windowTitleUpdateTime = 0.0f;
			avgFPSCount = 0;
		}

		// Swap the buffers so the game window updates with the new frame
		glfwSwapBuffers(game.window);
	}

	// Main loop has ended, exit game (no menu yet)
	game.mainLoopActive = false;
	ExitGame();
}

void Badcraft::TextUpdate()
{
	/*
		Even if the game runs with no framerate cap, there are a few things that
		are pointless to be done every frame, such as the dynamic information text;
		
		This would have no real benefit and could instead make it harder to read some
		values that change frequently.
	*/

	// Update dynamic information text
	std::string newInfoText = std::format(
		"{} FPS {} AVG {} LOW\n{:.2f} {:.2f} {:.2f} ({}, {}, {})\nY:{:.1f} P:{:.1f} S:{:.1f}\nT:{:.1f} ({:.1f})",
		m_nowFPS, m_avgFPS, m_lowFPS,
		player.position.x, player.position.y, player.position.z,
		player.offset.x, player.offset.y, player.offset.z,
		player.yaw, player.pitch, player.currentSpeed,
		game.currentFrameTime, game.tickedFrameTime
	);

	world.textRenderer.ChangeText(m_infoText, newInfoText);

	// Check the status of text objects to determine visibility
	world.textRenderer.CheckTextStatus();
	m_updateTime = 0.0; // Reset function wait timer
}

void Badcraft::ExitGame()
{
	// Since there is no main menu yet, the window can just be destroyed.

	TextFormat::log("Game exit");
	world.threadUpdateBuffers = false;
	game.test = false;

	// Join any created threads as there is no longer any need for them
	for (std::thread& thread : game.standaloneThreads) thread.join();

	// Close the game window and terminate GLFW and the program itself
	glfwDestroyWindow(game.window);
	glfwTerminate();
}

void Badcraft::UpdateAspect()
{
	// Get window dimensions as float
	const float w = static_cast<float>(game.width), 
		h = static_cast<float>(game.height);

	// Aspect ratio of screen
	game.aspect = w / h;

	// Calculate GUI sizes
	const float textWidth = 0.007f,
		textHeight = 0.08f,
		inventoryWidth = 1.0f,
		inventoryHeight = 1.5f;

	// Update the shader UBO values
	float sizesData[] = { game.aspect, textWidth, textHeight, inventoryWidth, inventoryHeight };
	OGL::UpdateUBO(m_sizesUBO, sizeof(float[2]), sizeof(sizesData), sizesData);

	world.textRenderer.letterSpacing = textWidth * 0.5f;
	world.textRenderer.lineSpacing = textHeight * 0.5f;
	world.textRenderer.RecalculateAllText();

	// Update perspective matrix and other perspective-related values
	UpdatePerspective();
}

void Badcraft::UpdatePerspective()
{
	// Update the perspective matrix with the current saved variables
	m_matrices[Perspective] = glm::perspective(
		static_cast<float>(player.fov), 
		game.aspect, 
		static_cast<float>(player.nearPlaneDistance),
		static_cast<float>(player.farPlaneDistance)
	);
	// Update other matrices on next frame to use new perspective matrix
	player.moved = true;
	// Update frustum culling plane values
	playerFunctions.UpdateFrustum();
}

void Badcraft::UpdateFrameValues()
{
	// Update positions UBO
	const double gamePositions[] = {
		// raycastBlockPosition
		static_cast<double>(player.targetBlockPosition.x),
		static_cast<double>(player.targetBlockPosition.y),
		static_cast<double>(player.targetBlockPosition.z), 1.0,
		// playerPosition
		player.position.x,
		player.position.y,
		player.position.z, 1.0
	};
	OGL::UpdateUBO(m_positionsUBO, 0, sizeof(gamePositions), gamePositions);

	// Time values for shaders
	const float crntFrame = static_cast<float>(game.tickedFrameTime);
	float m_gameDayTime = sinf((crntFrame * WorldSky::dayNightTimeReciprocal * Math::PI_FLT) - Math::HALF_PI) + 1.0f;

	const float skyLerp = m_gameDayTime * 0.5f;
	const float starAlpha = (m_gameDayTime - 0.8f) * 0.5f;

	// Partial matrix UBO update (perspective and other matrices are done on player movement)
	glm::mat4 starmatrix = m_matrices[Origin];
	starmatrix = glm::rotate(starmatrix, crntFrame * WorldSky::starRotationalSpeed, glm::vec3(0.96f, 0.54f, 0.2f));

	OGL::UpdateUBO(m_matricesUBO, sizeof(glm::mat4[2]), sizeof(starmatrix), &starmatrix);

	// Update time variables UBO
	const float gameTimes[] = {
		m_gameDayTime, // time
		1.0f - skyLerp, // fogTime
		starAlpha, // starTime
		crntFrame, // gameTime
		1.1f - (m_gameDayTime * 0.5f), // cloudsTime
	};
	OGL::UpdateUBO(m_timesUBO, 0, sizeof(gameTimes), gameTimes);

	constexpr glm::vec3 skyMorningColour = glm::vec3(0.45f, 0.72f, 0.98f);
	constexpr glm::vec3 skyTransitionColour = glm::vec3(1.0f, 0.45f, 0.0f);
	constexpr glm::vec3 skyNightColour = glm::vec3(0.0f, 0.0f, 0.05f);
	
	const glm::vec3 mainSkyColour = glm::vec3(
		Math::lerp(skyMorningColour.x, skyNightColour.x, skyLerp),
		Math::lerp(skyMorningColour.y, skyNightColour.y, skyLerp),
		Math::lerp(skyMorningColour.z, skyNightColour.z, skyLerp)
	);

	// Paint/clear background with sky colour instead of having a 
	// massive shape surrounding player (expensive fragment shader)
	glClearColor(mainSkyColour.x, mainSkyColour.y, mainSkyColour.z, 1.0f);
	const float eveningLerp = powf(1.0f - m_gameDayTime, 20.0f);

	const glm::vec3 eveningSkyColour = glm::vec3(
		Math::lerp(mainSkyColour.x, skyTransitionColour.x, eveningLerp),
		Math::lerp(mainSkyColour.y, skyTransitionColour.y, eveningLerp),
		Math::lerp(mainSkyColour.z, skyTransitionColour.z, eveningLerp)
	);
	const float worldLight = 1.1f - (m_gameDayTime * 0.5f);

	// Update colour vectors UBO
	const float gameColours[] = {
		// mainSkyColour
		mainSkyColour.x, mainSkyColour.y, mainSkyColour.z, 1.0f,
		// eveningSkyColour
		eveningSkyColour.x, eveningSkyColour.y, eveningSkyColour.z, 1.0f,
		// worldLight
		worldLight, worldLight, worldLight, 1.0f,
	};
	OGL::UpdateUBO(m_coloursUBO, 0, sizeof(gameColours), gameColours);
}

void Badcraft::TakeScreenshot()
{
	bool error = false;
	constexpr const char* screenshotfolder = "Screenshots";

	// Ensure screenshot folder is valid by either creating or replacing
	try {
		std::error_code ec;
		// Create if there is no folder
		if (!std::filesystem::exists(screenshotfolder)) std::filesystem::create_directory(screenshotfolder);
		// Replace if it somehow is a file instead of a folder
		else if (!std::filesystem::is_directory(screenshotfolder, ec)) {
			std::filesystem::remove(screenshotfolder);
			std::filesystem::create_directory(screenshotfolder);
		}
		error = ec.value() != 0;
	}
	catch (std::filesystem::filesystem_error exception) { error = true; }

	const char* newScreenshotText = "Failed to save screenshot";

	if (!error) {
		constexpr const size_t snprintfsz = static_cast<size_t>(200);
		char filenamebuf[snprintfsz], directorybuf[snprintfsz];

		// Get formatted time values
		tm lct;
		const time_t now = time(0); 
		localtime_s(&lct, &now);

		// Determine file name in above char buffers
		const float t = static_cast<float>(glfwGetTime());
		const int milliseconds = static_cast<int>((t - floor(t)) * 1000.0f);
		snprintf(filenamebuf, snprintfsz, "Screenshot %d-%d-%d %d-%02d-%02d-%03d.png",
			lct.tm_year + 1900, lct.tm_mon + 1, lct.tm_mday, lct.tm_hour, lct.tm_min, lct.tm_sec, milliseconds);
		snprintf(directorybuf, snprintfsz, "%s/%s", screenshotfolder, filenamebuf);

		// Create uint8_t vector of size 3 * width * height for each pixel and RGB values
		const unsigned w = game.width, h = game.height;
		std::vector<unsigned char> pixels(3u * w * h);

		// Read pixels from screen to vector
		glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
		
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

		// Save to PNG file
		const bool success = lodepng::encode(directorybuf, newHalf, w, h, LodePNGColorType::LCT_RGB) == 0;

		newHalf.clear();
		pixels.clear();

		// Create result text to be displayed
		if (success) {
			snprintf(directorybuf, snprintfsz, "Screenshot saved as %s", filenamebuf);
			newScreenshotText = static_cast<const char*>(directorybuf);
		}
	}

	world.textRenderer.ChangeText(m_screenshotText, newScreenshotText);
	m_screenshotText->type = TextRenderer::T_Type::TemporaryShow;
	m_screenshotText->ResetTextTime();
}

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
			// Hide the text object and remove the associated text
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

		return;
	}

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
void Badcraft::Callbacks::ToggleInventory() noexcept
{
	// Open or close the inventory
	b_not(app->player.inventoryOpened);
	glfwSetInputMode(game.window, GLFW_CURSOR, app->player.inventoryOpened ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	// Reset mouse position
	glfwSetCursorPos(game.window, static_cast<double>(game.width) / 2.0, static_cast<double>(game.height) / 2.0);
	game.focusChanged = true;
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
	int cmdSize = static_cast<int>(command.size());
	if (cmdSize < 1) return;

	if (command[0] != '/') {
		// TODO: normal chat
		return;
	}

	// Ensure there is only one space at the end of the command
	for (int i = cmdSize - 1; i >= 0; --i) {
		if (command[i] != ' ') {
			size_t rem = static_cast<size_t>(cmdSize - i - 1);
			command = command.substr(0, command.size() - rem);
			command += " ";
			break;
		}
	}

	// Get each argument from the command given using spaces
	std::vector<std::string> args;
	size_t last = oneSizeT;

	while (true) {
		size_t ind = command.find_first_of(" ", last);
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

	enum class PLP { X, Y, Z };
	typedef std::pair<int, PLP> ConversionPair;
	// Convert specific terms into their associated variables (e.g. '~' becomes a specific position axis)
	static const auto cvrt = [&](std::vector<ConversionPair> indpairs) {
		isConverting = true;
		for (ConversionPair& pair : indpairs) {
			const auto &[ind, axis] = pair;
			std::string& c_arg = getArg(ind);

			bool isSquigglyCharacterThing = c_arg.size() == 1 && c_arg[0] == '~';
			if (!isSquigglyCharacterThing) continue;

			switch (axis) {
				case PLP::X:
					c_arg = std::to_string(pos.x);
					break;
				case PLP::Y:
					c_arg = std::to_string(pos.y);
					break;
				case PLP::Z:
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
	static const std::unordered_map<std::string, func> commandsMap = {
		{ "tp", [&]() {
			argnum(3);
			cvrt({ { 0, PLP::X }, { 1, PLP::Y }, { 2, PLP::Z } });
			fplayer.SetPosition(glm::dvec3(getDbl(0), getDbl(1), getDbl(2)));
		}}, 
		{ "exit", [&]() {
			game.mainLoopActive = false;
		}}, 
		{ "help", [&]() {
			game.mainLoopActive = false; // TODO: ...actually help the player?
		}}, 
		{ "speed", [&]() {
			argnum(1);
			plr.defaultSpeed = getDbl(0);
		}},
		{ "tick", [&]() {
			argnum(1);
			game.tickSpeed = getDbl(0);
		}},
		{ "fill", [&]() {
			argnum(7);
			cvrt({ {0, PLP::X}, {1, PLP::Y}, {2, PLP::Z}, {3, PLP::X}, {4, PLP::Y}, {5, PLP::Z} });
			app->world.FillBlocks(
				getInt(0), getInt(1), getInt(2),
				getInt(3), getInt(4), getInt(5),
				getBlk(6)
			);
		}}, 
		{ "dcmp", [&]() {
			argnum(1);
			std::vector<char> binary(static_cast<size_t>(48 * 1024));
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
		catch (std::exception e) {
			TextFormat::warn(
				std::format(
					"Error during command parsing:\nArgument index: {}\nArgument: {}",
					currentArgumentCheck, currentArgument
				), 
				e.what()
			);
		}
	}
}
