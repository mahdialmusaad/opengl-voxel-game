#include "Game.hpp"

GameObject::GameObject() noexcept :
	world(player),
	player(playerFunctions.player),
	callbacks(this)
{
	TextFormat::log("Game init enter");
	playerFunctions.world = &world;

	int width, height;
	glfwGetWindowSize(game.window, &width, &height);

	glViewport(0, 0, width, height); // Ensure viewport is updated with initial window size

	// OGL rendering settings
	glEnable(GL_PROGRAM_POINT_SIZE); // Determine GL_POINTS size in shaders instead of from a global value
	glEnable(GL_DEPTH_TEST); // Enable depth testing so objects are rendered properly
	glDepthFunc(GL_LEQUAL); // Suitable depth testing function

	glEnable(GL_BLEND); // Transparency blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Suitable blend function

	glEnable(GL_CULL_FACE); // Enable face culling
	glCullFace(GL_BACK); // The inside of objects are not seen normally so they can be ignored in rendering

	// Setup buffers for shared program data (update size to reflect shader ubo size)
	OGL::SetupUBO(m_matricesUBO, 0, sizeof(glm::mat4[4]));
	OGL::SetupUBO(m_timesUBO, 1, sizeof(float[5]));
	OGL::SetupUBO(m_coloursUBO, 2, sizeof(glm::vec4[3]));
	OGL::SetupUBO(m_positionsUBO, 3, sizeof(glm::dvec4[2]));
	OGL::SetupUBO(m_sizesUBO, 4, sizeof(float[2]));

	// Update block texture size in shader UBO
	const float sizesData[] = {
		16.0f / static_cast<float>(game.blocksTextureInfo.width),
		1.0f / static_cast<float>(game.textTextureInfo.width)
	};
	OGL::UpdateUBO(m_sizesUBO, 0, sizeof(sizesData), sizesData);

	// Aspect ratio for GUI sizing and perspective matrix - do before text creation to calculate with updated values
	UpdateAspect();

	// Text objects creation
	TextRenderer &tr = world.textRenderer;
	const std::uint8_t bgShadow = TextRenderer::TS_Background | TextRenderer::TS_Shadow;
	const float left = -0.99f;

	// Combine static text for less draw calls and memory
	const std::string infofmt = fmt::format(
		"{}\n{}\nSeed: {}", 
		glfwGetVersionString(),
		OGL::GetString(GL_RENDERER), 
		game.noiseGenerators.continentalness.seed
	);
	TextRenderer::ScreenText *sText = tr.CreateText({ left, 0.99f }, infofmt, 12u, bgShadow | TextRenderer::TS_DebugText);
	
	m_chatText = tr.CreateText({ left, 0.0f }, "", 10u, bgShadow, TextRenderer::TextType::Hidden); // Chat log
	m_commandText = tr.CreateText({ left, -0.74f }, "", 12u, bgShadow | TextRenderer::TS_BackgroundFullX, TextRenderer::TextType::Hidden); // Command text

	// Dynamic texts updated constantly to show game information
	m_infoText = tr.CreateText({ left, GetRelativeTextPosition(sText) }, "", 12u, bgShadow | TextRenderer::TS_DebugText);
	m_infoText2 = tr.CreateText({ left, GetRelativeTextPosition(m_infoText, 6) }, "", 12u, bgShadow | TextRenderer::TS_DebugText);

	playerFunctions.WorldInitialize(); // Initialize player text and other variables

	// Move player to highest block on starting position
	const PosType startingAxis = static_cast<PosType>(ChunkSettings::CHUNK_SIZE_HALF);
	const PosType highestYPosition = world.HighestBlockPosition(startingAxis, startingAxis);
	const double chunkCenter = static_cast<double>(startingAxis) + 0.5;
	playerFunctions.SetPosition({ chunkCenter, static_cast<double>(highestYPosition) + 3.0, chunkCenter });

	// Clear all image data in game struct as they are no longer needed
	game.blocksTextureInfo.data.clear();
	game.textTextureInfo.data.clear();
	game.inventoryTextureInfo.data.clear();

	//DebugFunctionTest();

	TextFormat::log("Game init exit");
}

void GameObject::Main()
{
	double miscellaneousUpdate = 0.0; // The time since the last window title update
	double largestUpdateTime = 0.0; // The largest amount of time between two frames
	int avgFPSCount = 0; // Number of frames since last title update

	// Main game loop
	TextFormat::log("Main loop enter");
	game.mainLoopActive = true;
	
	while (game.mainLoopActive) {
		glfwPollEvents(); // Poll I/O such as key and mouse events

		// Time values
		game.deltaTime = glfwGetTime() - m_lastTime;
		game.tickedDeltaTime = game.deltaTime * game.tickSpeed;
		if (game.deltaTime > largestUpdateTime) largestUpdateTime = game.deltaTime;
		miscellaneousUpdate += game.deltaTime;
		m_updateTime += game.deltaTime;
		m_lastTime = game.deltaTime + m_lastTime;
		avgFPSCount++;
		game.daySeconds += game.tickedDeltaTime;
		if (game.daySeconds >= m_skybox.fullDaySeconds) { game.daySeconds = 0.0; ++game.worldDay; }

		playerFunctions.CheckInput(); // Check for per-frame inputs
		playerFunctions.ApplyMovement(); // Apply smoothed movement using velocity and other position-related functions
		if (player.moved) MoveMatricesUpdate(); // Update matrices, frustum, etc on position change

		UpdateFrameValues(); // Update shader UBO values (day/night cycle, sky colours)

		// Game rendering
		playerFunctions.RenderBlockOutline();
		world.DrawWorld();

		//m_skybox.RenderSky();
		m_skybox.RenderStars();
		m_skybox.RenderSunAndMoon();
		m_skybox.RenderClouds();

		if (game.showGUI) {
			glClear(GL_DEPTH_BUFFER_BIT); // Clear depth again to guarantee GUI draws on top
			playerFunctions.RenderPlayerGUI();
			world.textRenderer.RenderText(player.inventoryOpened);
		}

		//playerFunctions.RaycastTest();
		world.DebugChunkBorders(true);

		// Window title update
		if (miscellaneousUpdate >= 0.05f) {
			// Determine FPS values
			m_avgFPS = static_cast<int>(static_cast<double>(avgFPSCount) / miscellaneousUpdate);
			m_lowFPS = static_cast<int>(1.0 / largestUpdateTime);
			m_nowFPS = static_cast<int>(1.0 / game.deltaTime);

			// Update/check things periodically
			GameMiscUpdate();

			// Reset values
			largestUpdateTime = game.deltaTime;
			miscellaneousUpdate = 0.0f;
			avgFPSCount = 0;
		}

		// Swap the buffers so the game window updates with the new frame
		glfwSwapBuffers(game.window);
	}

	// Main loop has ended, exit game (no menu yet)
	game.mainLoopActive = false;
	ExitGame();
}

void GameObject::MoveMatricesUpdate() noexcept
{
	player.moved = false;
	
	// Update view matrix for complicated 3D maths to determine where something is on the screen
	playerFunctions.SetViewMatrix(m_matrices[Matrix_View]);
	m_matrices[Matrix_World] = m_matrices[Matrix_Perspective] * m_matrices[Matrix_View];

	playerFunctions.UpdateFrustum(); // Update frustum values for frustum culling
	world.SortWorldBuffers(); // Sort the world buffers to determine what needs to be rendered

	// Remove translation from origin matrix for specific positioning
	m_matrices[Matrix_Origin] = m_matrices[Matrix_Perspective] * glm::mat4(glm::mat3(m_matrices[Matrix_View]));

	OGL::UpdateUBO(m_matricesUBO, 0, sizeof(glm::mat4[2]), m_matrices + 2); // Update world matrix values in GPU shaders
}

void GameObject::GameMiscUpdate() noexcept
{
	// Check or update some things at a fixed rate, could be too expensive/unnecessary to do so every frame.
	
	// Update window title
	const std::string FPStext = fmt::format("{} FPS | {} AVG | {} LOW", m_nowFPS, m_avgFPS, m_lowFPS);
	glfwSetWindowTitle(game.window, FPStext.c_str());

	// Update dynamic information text
	static const char *directionText[6] = { "East", "West", "Up", "Down", "North", "South" };
	const glm::dvec3 &vel = playerFunctions.GetVelocity();
	
	bool isFarAway = Math::isLargeOffset(player.offset);
	std::string infofmt = fmt::format(
		"\n{} {} {}{}({} {} {})\nVelocity: {:.2f} {:.2f} {:.2f}\nYaw:{:.1f} Pitch:{:.1f} ({}, {})\nFOV:{:.1f} Speed:{:.1f}\nFlying: {} Noclip: {}",
		TextFormat::groupDecimal(player.position.x), TextFormat::groupDecimal(player.position.y), TextFormat::groupDecimal(player.position.z),
		isFarAway ? "\n" : " ",
		player.offset.x, player.offset.y, player.offset.z,
		vel.x, vel.y, vel.z,
		player.yaw, player.pitch,
		directionText[player.lookDirectionYaw], directionText[player.lookDirectionPitch],
		glm::degrees(player.fov), player.currentSpeed, !player.doGravity, player.noclip
	);

	std::string infofmt2 = fmt::format(
		"Chunks: {} R.Distance: {} Gen: {}\nTriangles: {} Ind.Calls: {}\nTime: {:.1f} (Day {})", 
		fmt::group_digits(world.allchunks.size()), world.chunkRenderDistance, !game.noGeneration,
		fmt::group_digits(world.squaresCount * 2u), fmt::group_digits(world.GetIndirectCalls()), game.daySeconds, game.worldDay
	);

	world.textRenderer.ChangeText(m_infoText, FPStext + infofmt); // Update first text info box

	// Update second text info box, determining if the Y position needs to be changed due to the above text being too long
	static bool wasFarAway = false;
	if (isFarAway != wasFarAway) world.textRenderer.ChangePosition(m_infoText2, { m_infoText2->GetPosition().x, GetRelativeTextPosition(m_infoText) }, false);
	world.textRenderer.ChangeText(m_infoText2, infofmt2, true);
	
	world.textRenderer.CheckTextStatus(); // Check the status of text objects to determine visibility
	wasFarAway = isFarAway; // Reset second text box position update check
	m_updateTime = 0.0; // Reset function wait timer
}

void GameObject::ExitGame() noexcept
{
	TextFormat::log("Game exit request");
	game.noGeneration = false;

	// Join any created threads as there is no longer any need for them
	for (std::thread &thread : game.standaloneThreads) thread.join();

	// All buffers (VBO, VAO, etc) are deleted in destructors, which is called at the end of the 'main' function.

	// Close the game window and terminate GLFW and the program itself
	glfwDestroyWindow(game.window);
	glfwTerminate();
}

double GameObject::EditTime(bool isSet, double t) noexcept
{
	if (isSet) {
		game.worldDay = static_cast<std::int64_t>(t / m_skybox.fullDaySeconds);
		game.daySeconds = t - (static_cast<double>(game.worldDay) * m_skybox.fullDaySeconds);
		return t;
	}
	else return game.daySeconds + (static_cast<double>(game.worldDay) * m_skybox.fullDaySeconds);
}

float GameObject::GetRelativeTextPosition(TextRenderer::ScreenText *sct, int linesOverride) noexcept
{
	TextRenderer &tr = world.textRenderer;
	return sct->GetPosition().y - (linesOverride == -1 ? tr.GetTextHeight(sct) : tr.GetTextHeight(sct->GetUnitSize(), linesOverride)) - 0.01f;
}

void GameObject::UpdateAspect() noexcept
{
	game.aspect = static_cast<float>(game.width) / static_cast<float>(game.height); // Calculate aspect ratio of new window size

	// Update text values
	world.textRenderer.textWidth = 0.007f / game.aspect;
	world.textRenderer.characterSpacingSize = world.textRenderer.textWidth;
	world.textRenderer.spaceCharacterSize = world.textRenderer.textWidth * 4.0f;
	world.textRenderer.RecalculateAllText();

	// Update player GUI
	playerFunctions.UpdateInventory();
	if (game.mainLoopActive) { // Ensure all text has been initialized first
		playerFunctions.UpdateInventoryTextPos();
		playerFunctions.UpdateBlockInfoText();
	} 

	UpdatePerspective(); // Update perspective matrix and other perspective-related values
}

void GameObject::UpdatePerspective() noexcept
{
	// Update the perspective matrix with the current saved variables
	m_matrices[Matrix_Perspective] = glm::perspective(
		player.fov,
		static_cast<double>(game.aspect),
		player.nearPlaneDistance,
		player.farPlaneDistance
	); 
	player.moved = true; // Update other matrices on next frame to use new perspective matrix
	playerFunctions.UpdateFrustum(); // Update frustum culling plane values
}

void GameObject::UpdateFrameValues() noexcept
{
	// Update positions UBO
	const double gamePositions[] = {
		// relativeRaycastBlockPosition
		static_cast<double>(player.targetBlockPosition.x) - player.position.x,
		static_cast<double>(player.targetBlockPosition.y) - player.position.y,
		static_cast<double>(player.targetBlockPosition.z) - player.position.z, 0.0,
		// playerPosition
		player.position.x,
		player.position.y,
		player.position.z, 0.0
	};
	OGL::UpdateUBO(m_positionsUBO, 0, sizeof(gamePositions), gamePositions);
	
	// Triangle wave from 0 to 1, reaching 1 at halfway through an in-game day and returning back to 0:
	// 2 * abs( (x / t) - floor( (x / t) + 0.5 ) ) where x is the current time and t is the time for one full day
	const double timeOverFullDay = game.daySeconds / static_cast<float>(m_skybox.fullDaySeconds);
	const float dayProgress = 2.0f * static_cast<float>(glm::abs(timeOverFullDay - glm::floor(timeOverFullDay + 0.5))); // 0 = midday, 1 = midnight

	// Update star and sun/moon matrices
	const glm::mat4 &originMatrix = m_matrices[Matrix_Origin];
	const float rotationAmount = game.daySeconds / (m_skybox.fullDaySeconds / (glm::pi<float>() * 2.0f));
	glm::mat4 newMatrices[2] = { 
		glm::rotate(originMatrix, rotationAmount, glm::vec3(0.7f, 0.54f, 0.2f)), // starMatrix
		glm::rotate(originMatrix, rotationAmount, glm::vec3(-1.0f, 0.0f, 0.0f))  // sunMoonMatrix
	};
	OGL::UpdateUBO(m_matricesUBO, sizeof(glm::mat4[2]), sizeof(newMatrices), &newMatrices);

	// Update time variables UBO
	const float starsThreshold = 0.4f;
	const float starAlpha = glm::clamp((dayProgress - starsThreshold) * (1.0f / (1.0f - starsThreshold)), 0.0f, 1.0f);

	const float gameTimes[] = {
		dayProgress, // time
		1.0f - dayProgress, // fogTime
		starAlpha, // starTime
		static_cast<float>(EditTime(false)), // gameTime
		1.1f - dayProgress, // cloudsTime
	};
	OGL::UpdateUBO(m_timesUBO, 0, sizeof(gameTimes), gameTimes);

	const glm::vec3 fullBrightColour = glm::vec3(0.45f, 0.72f, 0.98f), fullDarkColour = glm::vec3(0.0f, 0.0f, 0.05f);
	const glm::vec3 mainSkyColour = Math::lerp(fullBrightColour, fullDarkColour, dayProgress);
	
	// Clear background with the sky colour - other effects can be rendered on top
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(mainSkyColour.x, mainSkyColour.y, mainSkyColour.z, 1.0f);

	const glm::vec3 skyTransitionColour = glm::vec3(1.0f, 0.45f, 0.0f);
	const float eveningLerp = 1.0f - dayProgress;//glm::pow(1.0f - darknessProgress, 20.0f);
	const glm::vec3 eveningSkyColour = Math::lerp(mainSkyColour, skyTransitionColour, eveningLerp);

	// Update colour vectors UBO
	const float worldLight = 1.1f - dayProgress; // TODO: after lighting stuff, make this progress in stages
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

void GameObject::DebugFunctionTest() noexcept
{
	for (int i = 0, times = 1; i <= 6; ++i) {
		double begin = glfwGetTime();
		for (int j = 0; j < times; ++j) world.GetChunk({0,0,0});
		fmt::print("*** DEBUG PASS {}: {}s at {} loops ***\n", i + 1, glfwGetTime() - begin, fmt::group_digits(times));
		times *= 10;
	}
}

GameObject::~GameObject()
{
	// Delete each UBO
	const GLuint deleteUBOs[] = {
		static_cast<GLuint>(m_positionsUBO),
		static_cast<GLuint>(m_matricesUBO),
		static_cast<GLuint>(m_coloursUBO),
		static_cast<GLuint>(m_timesUBO),
		static_cast<GLuint>(m_sizesUBO),
	};

	glDeleteBuffers(static_cast<GLsizei>(Math::size(deleteUBOs)), deleteUBOs);
}
