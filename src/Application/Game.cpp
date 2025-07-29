#include "Game.hpp"

GameObject::GameObject() noexcept :
	player(playerFunctions.player),	
	world(player),
	callbacks(this)
{
	playerFunctions.world = &world;

	// Setup buffers for shared program data (update size to reflect shader ubo size)
	OGL::SetupUBO(m_matricesUBO, 0, sizeof(glm::mat4[4]));
	OGL::SetupUBO(m_timesUBO, 1, sizeof(float[6]));
	OGL::SetupUBO(m_coloursUBO, 2, sizeof(glm::vec4[3]));
	OGL::SetupUBO(m_positionsUBO, 3, sizeof(glm::dvec4[2]));
	OGL::SetupUBO(m_sizesUBO, 4, sizeof(float[3]));

	// Update 'sizes' UBO with values
	const float sizesData[] = {
		16.0f / static_cast<float>(game.blocksTextureInfo.width), // blockTextureSize
		1.0f / static_cast<float>(game.textTextureInfo.width), // inventoryTextureSize
		1.0f / static_cast<float>(m_skybox.numStars) // starsCount
	};
	OGL::UpdateUBO(m_sizesUBO, sizesData, sizeof(sizesData));

	// Aspect ratio for GUI sizing and perspective matrix - do before text creation to calculate with updated values
	UpdateAspect();
	
	// Text objects creation
	TextRenderer &tr = world.textRenderer;
	const std::uint8_t bgShadow = TextRenderer::TS_Background | TextRenderer::TS_Shadow;
	const std::uint8_t bgShadowDebug = bgShadow | TextRenderer::TS_DebugText;
	const float left = -0.99f;

	// Combine static text for less draw calls and memory
	const std::string infofmt = fmt::format(
		"{}\n{}\nSeed: {}", 
		glfwGetVersionString(),
		OGL::GetString(GL_RENDERER), 
		game.noiseGenerators.elevation.seed
	);

	const unsigned defsize = tr.defaultUnitSize;
	TextRenderer::ScreenText *staticText = tr.CreateText({ left, 0.99f }, infofmt, bgShadowDebug);
	
	m_chatText = tr.CreateText({ left, -0.15f }, "", bgShadow, defsize - 2u, TextRenderer::TextType::Hidden); // Chat log
	m_commandText = tr.CreateText({ left, -0.74f }, "", bgShadow | TextRenderer::TS_BackgroundFullX, defsize, TextRenderer::TextType::Hidden); // Command text

	// Dynamic texts updated constantly to show game information (Y pos and text determined in game loop)
	m_infoText = tr.CreateText({ left, tr.GetRelativeTextYPos(staticText) }, "", bgShadowDebug); // Can get Y pos, uses static text
	m_infoText2 = tr.CreateText({ left, 0.0f }, "", bgShadowDebug);
	m_perfText = tr.CreateText({ left, 0.0f }, "", bgShadowDebug); // Performance text

	playerFunctions.WorldInitialize(); // Initialize player text and other variables

	// Move player to highest block on starting position
	const PosType startingAxis = static_cast<PosType>(ChunkValues::size / 2);
	const PosType highestYPosition = world.HighestBlockPosition(startingAxis, startingAxis);
	const double chunkCenter = static_cast<double>(startingAxis) + 0.5;
	playerFunctions.SetPosition({ chunkCenter, static_cast<double>(highestYPosition) + 3.0, chunkCenter });

	// Clear all image data in game struct as they are no longer needed
	game.blocksTextureInfo.data.clear();
	game.textTextureInfo.data.clear();
	game.inventoryTextureInfo.data.clear();

	// PerlinResultTest();
}

void GameObject::InitGame(char *location)
{
	// Variable initialization and settings that only need to be applied once

	// OGL rendering settings
	glEnable(GL_PROGRAM_POINT_SIZE); // Determine GL_POINTS size in shaders instead of from a global value
	glEnable(GL_DEPTH_TEST); // Enable depth testing so objects are rendered properly
	glDepthFunc(GL_LEQUAL); // Suitable depth testing function

	glEnable(GL_BLEND); // Transparency blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Suitable blend function

	glEnable(GL_CULL_FACE); // Enable face culling
	glCullFace(GL_BACK); // The inside of objects are not seen normally so they can be ignored in rendering
	
	// Utility functions
	glfwSetWindowSizeLimits(game.window, 300, 200, GLFW_DONT_CARE, GLFW_DONT_CARE); // Enforce minimum window size
	glfwSetInputMode(game.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Disable moving the cursor so it doesn't go outside the window
	glfwSwapInterval(1); // Swap buffers with screen's refresh rate
	
	game.threads = glm::max(static_cast<int>(std::thread::hardware_concurrency()), 1); // Get number of threads (min 1)
	
	// Get current directory for finding resources
	game.currentDirectory = location;
	FileManager::GetParentDirectory(game.currentDirectory);
	
	// Attempt to find resources folder - located in the same directory as exe
	const std::string resFolder = game.currentDirectory + "\\Resources\\";
	if (FileManager::DirectoryExists(resFolder)) game.resourcesFolder = resFolder;
	else throw std::runtime_error("Resources folder not found");

	// Load game textures with specific formats and save information about them
	FileManager::LoadImage(game.blocksTextureInfo, "Textures\\atlas.png");
	FileManager::LoadImage(game.textTextureInfo, "Textures\\textAtlas.png");//, ImageInfo::Format(GL_RED, LodePNGColorType::LCT_GREY, 1u));
	FileManager::LoadImage(game.inventoryTextureInfo, "Textures\\inventory.png");//, ImageInfo::Format(GL_RED, LodePNGColorType::LCT_GREY));

	shader.InitShaders(); // Initialize shader class
	ChunkLookupData::CalculateLookupData(chunkLookupData); // Initialize chunk terrain calculation lookup data

	TextFormat::log("Game init complete");
}

void GameObject::Main()
{
	double miscellaneousUpdate = 1.0; // The time since the last window title update
	double largestUpdateTime = 0.0; // The largest amount of time between two frames
	int avgFPSCount = 0; // Number of frames since last title update

	// Main game loop
	TextFormat::log("Main loop enter");
	game.mainLoopActive = true;
	
	while (game.mainLoopActive) {
		// Poll I/O such as key and mouse events
		glfwPollEvents();

		// Time values
		game.deltaTime = glfwGetTime() - m_lastTime;
		game.tickedDeltaTime = game.deltaTime * game.tickSpeed;
		largestUpdateTime = glm::max(game.deltaTime, largestUpdateTime);
		miscellaneousUpdate += game.deltaTime;
		m_updateTime += game.deltaTime;
		m_lastTime = game.deltaTime + m_lastTime;
		avgFPSCount++;
		game.daySeconds += game.tickedDeltaTime;
		if (game.daySeconds >= m_skybox.fullDaySeconds) { game.daySeconds = 0.0; ++game.worldDay; }

		playerFunctions.CheckInput(); // Check for per-frame inputs
		playerFunctions.ApplyMovement(); // Apply smoothed movement using velocity and other position-related functions
		if (player.moved) MovedUpdate(); // Update matrices and frustum on position change

		UpdateFrameValues(); // Update shader UBO values (day/night cycle, sky colours)

		// Game rendering
		playerFunctions.RenderBlockOutline();
		world.DrawWorld();

		m_skybox.RenderSky();
		m_skybox.RenderStars();
		m_skybox.RenderPlanets();
		m_skybox.RenderClouds();

		world.DebugChunkBorders(true);

		if (game.showGUI) {
			glClear(GL_DEPTH_BUFFER_BIT); // Clear depth again to ensure GUI draws on top
			playerFunctions.RenderPlayerGUI();
			world.textRenderer.RenderText(player.inventoryOpened);
		}

		// Window title update
		if (miscellaneousUpdate >= 0.05f) {
			// Determine FPS values
			m_avgFPS = static_cast<int>(static_cast<double>(avgFPSCount) / miscellaneousUpdate);
			m_lowFPS = static_cast<int>(1.0 / largestUpdateTime);
			m_nowFPS = static_cast<int>(1.0 / game.deltaTime);

			// Update or check things periodically
			MiscUpdate();

			// Reset values
			largestUpdateTime = game.deltaTime;
			miscellaneousUpdate = 0.0f;
			avgFPSCount = 0;
		}

		// Swap the buffers so the game window updates with the new frame
		glfwSwapBuffers(game.window);
	}

	// Main loop has ended, exit game
	game.mainLoopActive = false;
	ExitGame();
}

void GameObject::MiscUpdate() noexcept
{
	// Check or update some things at a fixed rate, could be too expensive/unnecessary to do so every frame.
	
	// Update window title
	const std::string FPStext = fmt::format("{} FPS | {} AVG | {} LOW", m_nowFPS, m_avgFPS, m_lowFPS);
	glfwSetWindowTitle(game.window, FPStext.c_str());

	const bool isFarAway = Math::isLargeOffset(player.offset); // Determine text spacing
	const glm::dvec3 &vel = playerFunctions.GetVelocity(); // Velocity shortcut

	// Update dynamic information text

	static const std::string infoFmtText = "\n{} {} {}{}({} {} {})\nVelocity: {:.2f} {:.2f} {:.2f}\nYaw:{:.1f} Pitch:{:.1f} ({}, {})\nFOV:{:.1f} Speed:{:.1f}\nFlying: {} Noclip: {}";
	world.textRenderer.ChangeText(m_infoText, FPStext + fmt::format(infoFmtText,
		TextFormat::groupDecimal(player.position.x), TextFormat::groupDecimal(player.position.y), TextFormat::groupDecimal(player.position.z),
		isFarAway ? "\n" : " ",
		player.offset.x, player.offset.y, player.offset.z,
		vel.x, vel.y, vel.z,
		player.yaw, player.pitch,
		game.constants.directionText[player.lookDirectionYaw], game.constants.directionText[player.lookDirectionPitch],
		glm::degrees(player.fov), player.currentSpeed, !player.doGravity, player.noclip
	)); // Update first text info box
	
	// Determine if Y position for second info box needs to change due to large position/offset values
	static bool wasFarAway = !isFarAway;
	const bool isDifferent = isFarAway != wasFarAway;
	if (isDifferent) world.textRenderer.ChangePosition(m_infoText2, { m_infoText2->GetPosition().x, world.textRenderer.GetRelativeTextYPos(m_infoText) }, false);
	static const std::string infoFmt2Text = "Chunks: {} (Rendered: {})\nTriangles: {} (Rendered: {})\nRenderDist: {} Generating: {} Ind.Calls: {}\nTime: {:.1f} (Day {})";
	world.textRenderer.ChangeText(m_infoText2, fmt::format(infoFmt2Text, 
		fmt::group_digits(world.allchunks.size()), fmt::group_digits(world.renderChunksCount),
		fmt::group_digits(world.squaresCount * 2u), fmt::group_digits(world.renderSquaresCount * 2u),
		world.chunkRenderDistance, !game.noGeneration, fmt::group_digits(world.GetIndirectCalls()),
		game.daySeconds, game.worldDay
	)); // Update second text info box

	// Sort top 5 functions based on performance
	std::sort(game.perfPointers, game.perfPointers + game.perfPointersCount, [&](GamePerfTest a, GamePerfTest b) { return a->total > b->total; });
	std::string perfFmt;
	for (int i = 0; i < 5; ++i) {
		GamePerfTest perf = game.perfPointers[i];
		perfFmt += fmt::format("{}. {} T:{:.3f}ms R:{}{}", i + 1, perf->name, perf->total, perf->count, i == 4 ? "" : "\n");
	}

	// Also update Y position for performance text, must have second info box text updated to get its height
	if (isDifferent) world.textRenderer.ChangePosition(m_perfText, { m_perfText->GetPosition().x, world.textRenderer.GetRelativeTextYPos(m_infoText2) }, false);
	world.textRenderer.ChangeText(m_perfText, perfFmt); // Update performance text
	
	world.textRenderer.CheckTextStatus(); // Check the status of text objects to determine visibility
	wasFarAway = isFarAway; // Reset second text box position update check
	m_updateTime = 0.0; // Reset function wait timer
}

void GameObject::ExitGame() noexcept
{
	TextFormat::log("Game exit");

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

void GameObject::UpdateAspect() noexcept
{
	game.aspect = static_cast<float>(game.width) / static_cast<float>(game.height); // Calculate aspect ratio of new window size

	// Update text values
	world.textRenderer.textWidth = world.textRenderer.defTextWidth / game.aspect;
	world.textRenderer.characterSpacingSize = world.textRenderer.textWidth;
	world.textRenderer.spaceCharacterSize = world.textRenderer.textWidth * 4.0f;
	world.textRenderer.RecalculateAllText();

	playerFunctions.UpdateInventory(); // Update player GUI

	if (game.mainLoopActive) { // Ensure all text has been initialized first
		playerFunctions.UpdateInventoryTextPos();
		playerFunctions.UpdateBlockInfoText();
	} 

	playerFunctions.UpdateFrustum(); // Since the aspect changed, the frustum planes need to be recalculated.
	player.moved = true; // Force matrices update, which use the window aspect.
}

void GameObject::MovedUpdate() noexcept
{
	// Calculate perspective and origin (no translation) matrices
	m_perspectiveMatrix = glm::mat4(glm::perspective(
		player.fov,
		static_cast<double>(game.aspect),
		player.frustum.nearPlaneDistance,
		player.frustum.farPlaneDistance
	)); 
	glm::mat4 originMatrix = m_perspectiveMatrix * playerFunctions.GetZeroMatrix();
	OGL::UpdateUBO(m_matricesUBO, &originMatrix, sizeof(glm::mat4)); // Set UBO matrix value

	world.SortWorldBuffers(); // Sort the world buffers to determine what needs to be rendered
	player.moved = false; // Use to check for next matrix and world buffer update
}

void GameObject::UpdateFrameValues() noexcept
{
	game.perfs.frameVals.Start();

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
	OGL::UpdateUBO(m_positionsUBO, gamePositions, sizeof(gamePositions));
	
	// Triangle wave from 0 to 1, reaching 1 at halfway through an in-game day and returning back to 0:
	// 2 * abs( (x / t) - floor( (x / t) + 0.5 ) ) where x is the current time and t is the time for one full day
	const double timeOverFullDay = game.daySeconds / static_cast<float>(m_skybox.fullDaySeconds);
	const float dayProgress = 2.0f * static_cast<float>(glm::abs(timeOverFullDay - glm::floor(timeOverFullDay + 0.5))); // 0 = midday, 1 = midnight
	
	// Update star and sun/moon matrices
	const glm::mat4 zeroMatrix = m_perspectiveMatrix * playerFunctions.GetZeroMatrix(); // 'Zero' matrix so skybox elements always appear around camera
	const float rotationAmount = game.daySeconds / (m_skybox.fullDaySeconds / glm::two_pi<float>());
	
	glm::mat4 newMats[2] = {
		glm::rotate(zeroMatrix, rotationAmount, glm::vec3(0.7f, 0.54f, 0.2f)), // starMatrix (edited below)
		glm::rotate(zeroMatrix, rotationAmount, glm::vec3(-1.0f, 0.0f, 0.0f)) // planetsMatrix
	};
	OGL::UpdateUBO(m_matricesUBO, newMats, sizeof(glm::mat4[2]), sizeof(glm::mat4));

	// Update time variables UBO
	const float starsThreshold = 0.4f;
	const float starAlpha = glm::clamp((dayProgress - starsThreshold) * (1.0f / (1.0f - starsThreshold)), 0.0f, 1.0f);

	// Fog variables
	const float floatsz = static_cast<float>(ChunkValues::size), rndDst = static_cast<float>(world.chunkRenderDistance);
	const float fogEnd = ((rndDst * 0.68f) * floatsz) + (dayProgress * -rndDst) + (static_cast<float>(game.hideFog) * 1e20f);

	const float gameTimes[] = {
		dayProgress, // time
		fogEnd, // fogEnd
		rndDst, // fogRange
		starAlpha, // starTime
		static_cast<float>(EditTime(false)), // gameTime
		1.1f - dayProgress, // cloudsTime
	};
	OGL::UpdateUBO(m_timesUBO, gameTimes, sizeof(gameTimes));

	const glm::vec3 fullBrightColour = glm::vec3(0.45f, 0.72f, 0.98f), fullDarkColour = glm::vec3(0.0f, 0.0f, 0.05f);
	const glm::vec3 mainSkyColour = Math::lerp(fullBrightColour, fullDarkColour, glm::pow(dayProgress, 2.0f) * (3.0f - 2.0f * dayProgress));
	
	// Clear background with the sky colour - other effects can be rendered on top
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(mainSkyColour.x, mainSkyColour.y, mainSkyColour.z, 1.0f);

	const glm::vec3 skyTransitionColour = glm::vec3(1.0f, 0.45f, 0.0f);
	const float eveningLerp = 1.0f - dayProgress;
	const glm::vec3 eveningSkyColour = Math::lerp(mainSkyColour, skyTransitionColour, eveningLerp);

	// Update colour vectors UBO
	const float worldLight = 1.1f - dayProgress; // TODO: after lighting stuff, make this progress in stages
	const float gameColours[] = {
		mainSkyColour.x, mainSkyColour.y, mainSkyColour.z, 0.0f, // mainSkyColour
		eveningSkyColour.x, eveningSkyColour.y, eveningSkyColour.z, 1.0f, // eveningSkyColour
		worldLight, worldLight, worldLight, 1.0f, // worldLight

	};
	OGL::UpdateUBO(m_coloursUBO, gameColours, sizeof(gameColours));

	game.perfs.frameVals.End();
}

void GameObject::DebugFunctionTest() noexcept
{
	// Test function performance
	const int times = std::pow(10, 6);
	const double begin = glfwGetTime();
	for (int i = 0; i < times; ++i) world.GetChunk(WorldPosition{});
	const double functime = glfwGetTime() - begin;
	fmt::print("*** {}s at {} loop(s) - {} funcs/s ***\n", functime, fmt::group_digits(times), TextFormat::groupDecimal(times / functime));
}

void GameObject::PerlinResultTest() const noexcept
{
	// Test perlin noise results by creating an image
	const unsigned d = 1000u;
	std::vector<unsigned char> imgdata(static_cast<std::size_t>(d * d));
	const double factor = 0.01;

	float min = 100.0, max = -min;
	bool octave = true;

	for (unsigned i{}; i < d; ++i) {
		for (unsigned j{}; j < d; ++j) {
			float res = octave ? game.noiseGenerators.elevation.GetOctave(
				static_cast<double>(i) * factor,
				NoiseValues::defaultY,
				static_cast<double>(j) * factor, 3
			) : game.noiseGenerators.elevation.GetNoise(
				static_cast<double>(i) * factor,
				NoiseValues::defaultY,
				static_cast<double>(j) * factor
			);
			const unsigned char pix = static_cast<unsigned char>(res * 255.0);
			imgdata[(i*d)+j] = 255u - pix;
			min = glm::min(min, res); max = glm::max(max, res);
		}
	}

	fmt::println("Min: {} Max: {}", min, max);

	lodepng::encode("perlin.png", imgdata, d, d, LodePNGColorType::LCT_GREY);
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
