#include "Player.hpp"

Player::Player() noexcept
{
	TextFormat::log("Player enter");

	// Temporary inventory test
	for (PlayerObject::InventorySlot& s : player.inventory) { 
		s.objectID = static_cast<ObjectID>((rand() % 8) + 1);
		s.count = static_cast<std::uint8_t>((rand() % (PlayerObject::maxSlotCount + 1)) + 1);
	}

	// Setup VAO and VBO for selection outline
	m_outlineVAO = OGL::CreateVAO();
	glEnableVertexAttribArray(0u);

	// To ensure that the block selection outline is always visible, a slightly *smaller* square outline 
	// is rendered slightly *in front* of each block face to avoid Z-fighting and block clipping

	constexpr float front1 = 1.002f;
	constexpr float front0 = -0.002f;

	constexpr float small1 = 0.998f;
	constexpr float small0 = 0.002f;

	static constexpr const float cubeCoordinates[] = {
		// Top face
		small0, front1, small0,
		small1, front1, small0,
		small1, front1, small1,
		small0, front1, small1,
		// Bottom face
		small1, front0, small0,
		small0, front0, small0,
		small0, front0, small1,
		small1, front0, small1,
		// Right face
		front1, small0, small1,
		front1, small0, small0,
		front1, small1, small0,
		front1, small1, small1,
		// Left face
		front0, small0, small1,
		front0, small0, small0,
		front0, small1, small0,
		front0, small1, small1,
		// Front face
		small0, small1, front1,
		small0, small0, front1,
		small1, small0, front1,
		small1, small1, front1,
		// Back face
		small0, small1, front0,
		small0, small0, front0,
		small1, small0, front0,
		small1, small1, front0,
	};

	// VBO with line float data
	m_outlineVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, sizeof(float[3]), nullptr);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(cubeCoordinates), cubeCoordinates, 0u);

	// Setup VAO for inventory
	m_inventoryVAO = OGL::CreateVAO();

	// Enable attributes 0-2: 0 = vec4, 1 = int, 2 = base vec3
	glEnableVertexAttribArray(0u);
	glEnableVertexAttribArray(1u);
	glEnableVertexAttribArray(2u);

	// Attribute divisors for instanced rendering
	glVertexAttribDivisor(0u, 1u);
	glVertexAttribDivisor(1u, 1u);

	// VBO for the inventory texture data
	m_inventoryIVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);

	// Attribute layout (location 0 and 1)
	// A0 (vec4): X, Y, W, H
	// A1 (int): Texture type - text or block (1st bit), inner texture id (other bits)
	constexpr GLsizei stride = sizeof(float[4]) + sizeof(std::int32_t);
	glVertexAttribPointer(0u, 4, GL_FLOAT, GL_FALSE, stride, nullptr);
	glVertexAttribIPointer(1u, 1, GL_UNSIGNED_INT, stride, reinterpret_cast<const void*>(sizeof(float[4])));

	// Inventory element quad vertex data
	struct QuadVertex {
		constexpr QuadVertex(float a, float b, float c) noexcept : a(a), b(b), c(c) {}
		float a, b, c;
	} constexpr const quadVerticesData[4] = {
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f },
		{ 1.0f, 1.0f, 0.0f },
		{ 1.0f, 0.0f, 1.0f }
	};

	// VBO for the quad vertices for inventory elements (location 2)
	m_inventoryQuadVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(2u, 3, GL_FLOAT, GL_FALSE, sizeof(float[3]), nullptr);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(quadVerticesData), quadVerticesData, 0u);

	UpdateCameraVectors();
	UpdateOffset();
	// Inventory is updated in aspect function, which is done at start anyways

	TextFormat::log("Player exit");
}

void Player::InitializeText() noexcept
{
	// Create text objects as now the text renderer class has been initialized

	// Selected block information text - created in player class instead of
	// with other text objects so it can be updated only when the selected block changes
	m_blockText = world->textRenderer.CreateText(
		glm::vec2(1.0f, 0.01f), "", 12u,
		TextRenderer::TS_Background | TextRenderer::TS_Shadow | TextRenderer::TS_InventoryInvisible
	);
	RaycastBlock(); // Raycast to force update with selected block data

	// Inventory item counters - rendered seperately from other text for display order
	for (int slotIndex = 0; slotIndex < 9; ++slotIndex) {
		PlayerObject::InventorySlot& slot = player.inventory[slotIndex];
		slot.slotText = world->textRenderer.CreateText(
			{ 0.281f + (slotIndex * 0.05f), 0.968f },
			std::to_string(slot.count), 10u,
			TextRenderer::TS_Shadow | TextRenderer::TS_InventoryInvisible
		);
	}
}

void Player::InventoryTextTest() noexcept
{
	for (int slotIndex = 0; slotIndex < 9; ++slotIndex) {
		PlayerObject::InventorySlot& slot = player.inventory[slotIndex];
		world->textRenderer.ChangePosition(slot.slotText, { game.testvals.x + (slotIndex * game.testvals.y), game.testvals.z });
	}
}

void Player::CheckInput() noexcept 
{
	// Check for any input that requires holding

	// Get key state
	const auto CheckKey = [](int key, int action = GLFW_PRESS) noexcept {
		return glfwGetKey(game.window, key) == action;
	};

	// Only check if there is a key being pressed and if the player isn't chatting
	if (game.anyKeyPressed && !game.chatting) {
		// Movement inputs
		if (CheckKey(GLFW_KEY_W)) ProcessKeyboard(WorldDirection::Front);
		if (CheckKey(GLFW_KEY_S)) ProcessKeyboard(WorldDirection::Back);
		if (CheckKey(GLFW_KEY_A)) ProcessKeyboard(WorldDirection::Left);
		if (CheckKey(GLFW_KEY_D)) ProcessKeyboard(WorldDirection::Right);

		// Noclip/flying mode inputs
		bool alreadyControl = false;
		if (!player.doGravity) {
			if (CheckKey(GLFW_KEY_SPACE)) ProcessKeyboard(WorldDirection::Up);
			if (CheckKey(GLFW_KEY_LEFT_CONTROL)) { ProcessKeyboard(WorldDirection::Down); alreadyControl = true; }
		}

		// Speed modifiers
		if (CheckKey(GLFW_KEY_LEFT_SHIFT)) player.currentSpeed = player.defaultSpeed * 2.0f;
		else if (!alreadyControl && CheckKey(GLFW_KEY_LEFT_CONTROL)) player.currentSpeed = player.defaultSpeed * 0.5f;
		else player.currentSpeed = player.defaultSpeed;
	}
}

void Player::UpdateMovement() noexcept
{
	// Player should slowly come to a stop instead of immediately stopping when no movement is applied
	// TODO: Framerate-independent lerp/smoothing
	const double movementLerp = std::clamp(game.deltaTime * 5.0, 0.0, 1.0);
	m_velocity.x = Math::lerp(m_velocity.x, 0.0, movementLerp);
	m_velocity.y = Math::lerp(m_velocity.y, 0.0, movementLerp);
	m_velocity.z = Math::lerp(m_velocity.z, 0.0, movementLerp);

	// Gravity should have a maximum instead of increasing indefinetly (terminal velocity)
	m_velocity.y = fmax(m_velocity.y + (player.gravity * game.deltaTime * player.doGravity), player.maxGravity);

	/*
		For each direction (per axis):

		Get the block found at the new position added with the relative position,
		(checking for 0.1 blocks in each direction for more accurate collisions)

		Check if the found block is solid,

		and remove velocity in that direction if their signs match
		(e.g. don't remove positive X velocity if a block was found in negative X direction)
	*/

	constexpr double plrWidth = 0.2;
	constexpr double playerHeight = 1.7;

	struct CollisionInfo {
		constexpr CollisionInfo(glm::dvec3 p, bool pos) noexcept :
			direction(p), positiveAxis(pos) {
		}
		const glm::dvec3 direction;
		const bool positiveAxis;
	}
	// Lists of all relative block coordinates to check for collision, optimized to check 
	// the most common scenarios first and seperated by axis to skip uneccessary checks
	// e.g. if a player collided with a block on their right, there is no need to check
	// for collisions on the left and vice versa
	constexpr XCollisionChecks[4] = {
		{ {  plrWidth, -1.0,  		0.0 }, true  },	// On the right at feet height
		{ { -plrWidth, -1.0,  		0.0 }, false },	// On the left at feet height
		{ {  plrWidth,  0.0,  		0.0 }, true  },	// On the right at head height
		{ { -plrWidth,  0.0,  		0.0 }, false },	// On the left at head height
	}, YCollisionChecks[2] = {
		{ { 0.0, -playerHeight, 	0.0 }, false },	// Below feet
		{ { 0.0,  plrWidth,  		0.0 }, true  },	// Above head
	}, ZCollisionChecks[4] = {
		{ { 0.0, -1.0,    	   plrWidth }, true  },	// In front at feet height
		{ { 0.0, -1.0,	  	  -plrWidth }, false },	// Behind at feet height
		{ { 0.0,  0.0,  	   plrWidth }, false },	// In front at head height
		{ { 0.0,  0.0, 		  -plrWidth }, true  },	// Behind at head height
	};

	// The new position to check collisions for
	const glm::dvec3 newPosition = player.position + m_velocity;

	// Collision checks in each axis:
	// X axis
	for (const CollisionInfo& colInfo : XCollisionChecks) {
		const glm::dvec3 checkPosition = newPosition + colInfo.direction;
		const WorldPos pos = Math::toWorld(checkPosition);
		if (ChunkSettings::GetBlockData(world->GetBlock(pos.x, pos.y, pos.z)).isSolid) {
			if ((m_velocity.x >= 0.0) == colInfo.positiveAxis) {
				m_velocity.x = 0.0;
				break;
			}
		}
	}
	// Y axis
	for (const CollisionInfo& colInfo : YCollisionChecks) {
		const glm::dvec3 checkPosition = newPosition + colInfo.direction;
		const WorldPos pos = Math::toWorld(checkPosition);
		if (ChunkSettings::GetBlockData(world->GetBlock(pos.x, pos.y, pos.z)).isSolid) {
			if ((m_velocity.y >= 0.0) == colInfo.positiveAxis) {
				m_velocity.y = 0.0;
				break;
			}
		}
	}
	// Z axis
	for (const CollisionInfo& colInfo : ZCollisionChecks) {
		const glm::dvec3 checkPosition = newPosition + colInfo.direction;
		const WorldPos pos = Math::toWorld(checkPosition);
		if (ChunkSettings::GetBlockData(world->GetBlock(pos.x, pos.y, pos.z)).isSolid) {
			if ((m_velocity.z >= 0.0) == colInfo.positiveAxis) {
				m_velocity.z = 0.0;
				break;
			}
		}
	}
	
	// Add current velocity to position
	player.position += m_velocity;

	// Update variables and other data affected by position
	UpdatePositionalVariables();
}

void Player::SetPosition(glm::dvec3 newPos) noexcept
{
	// Set the new position of the player
	player.position = newPos;
	UpdatePositionalVariables();
}

const glm::dvec3 &Player::GetVelocity() const noexcept
{
	return m_velocity;
}

void Player::UpdatePositionalVariables() noexcept
{
	player.moved = true;
	UpdateOffset();
	RaycastBlock();
}

void Player::BreakBlock() noexcept
{
	// Check if the selected block is solid/valid
	if (ChunkSettings::GetBlockData(player.targetBlock).isSolid) {
		// Set broken block to air and redo raycast
		world->SetBlock(player.targetBlockPosition.x, player.targetBlockPosition.y, player.targetBlockPosition.z, ObjectID::Air);
		RaycastBlock();

		// Update inventory with new item
		const int presentIndex = SearchForItemNonFull(player.targetBlock);

		if (presentIndex == -1) {
			// Target block was not found in inventory or the found was/were full
			const int airIndex = SearchForItem(ObjectID::Air);
			if (airIndex == -1) return; // No empty or same-type slots were found
			else {
				// Replace empty slot with item
				PlayerObject::InventorySlot& emptySlot = player.inventory[airIndex];
				emptySlot.count = 1u;
				emptySlot.objectID = player.targetBlock;
				world->textRenderer.ChangeText(emptySlot.slotText, "1");
			}
		} else {
			// A non-full slot with the same type was found
			PlayerObject::InventorySlot& foundSlot = player.inventory[presentIndex];
			world->textRenderer.ChangeText(foundSlot.slotText, std::to_string(++foundSlot.count));
			UpdateInventory();
		}
	}
}

ModifyWorldResult Player::PlaceBlock() noexcept
{
	// Determine selected block in inventory
	PlayerObject::InventorySlot& useSlot =  player.inventory[selected];
	const ObjectID placeBlock = useSlot.objectID;

	// Only place if the current selected block is valid
	if (placeBlock != ObjectID::Air && ChunkSettings::GetBlockData(player.targetBlock).isSolid) {
		world->SetBlock(
			player.targetBlockPosition.x, 
			player.targetBlockPosition.y,
			player.targetBlockPosition.z,
			placeBlock
		);

		RaycastBlock();

		// Decrement item amount, changing to air if it was the last one
		if (--useSlot.count == 0u) { 
			useSlot.objectID = ObjectID::Air;
			UpdateInventory();
		}

		// Update text if needed
		useSlot.slotText->textType = useSlot.count > 0u ? TextRenderer::TextType::Default : TextRenderer::TextType::Hidden;
		world->textRenderer.ChangeText(useSlot.slotText, std::to_string(useSlot.count));
	}

	// Temporary
	return ModifyWorldResult::Passed;
}

void Player::ProcessKeyboard(WorldDirection direction) noexcept
{
	const double directionMultiplier = player.currentSpeed * game.deltaTime;
	const double previousYVelocity = m_velocity.y;

	// Add to velocity based on frame time and input
	switch (direction) {
		// Player movement
		case WorldDirection::Front:
			m_velocity += m_NPcamFront * directionMultiplier;
			break;
		case WorldDirection::Back:
			m_velocity += m_NPcamFront * -directionMultiplier;
			break;
		case WorldDirection::Left:
			m_velocity += m_NPcamRight * -directionMultiplier;
			break;
		case WorldDirection::Right:
			m_velocity += m_NPcamRight * directionMultiplier;
			break;
		// Flying movement
		case WorldDirection::Up:
			m_velocity.y += directionMultiplier;
			break;
		case WorldDirection::Down:
			m_velocity.y += -directionMultiplier;
			break;
		// Jumping logic
		case WorldDirection::None:
			// Lazy 'grounded' check
			if (abs(m_velocity.y) > 0.01) break;
			m_velocity.y += player.jumpPower;
			break;
		default:
			break;
	}

	// Remove any Y velocity change as a result of moving in camera direction if gravity is enabled
	if (player.doGravity) m_velocity.y = previousYVelocity;
}

int Player::SearchForItem(ObjectID item) noexcept
{
	// Return index of item in inventory or -1 if none was found
	for (int i = 0; i < 9; ++i) if (player.inventory[i].objectID == item) return i;
	return -1;
}

int Player::SearchForItemNonFull(ObjectID item) noexcept
{
	// Same as above, but will only return if the slot is not full
	for (int i = 0; i < 9; ++i) if (player.inventory[i].objectID == item && player.inventory[i].count < PlayerObject::maxSlotCount) return i;
	return -1;
}

void Player::RenderBlockOutline() const noexcept
{
	// Only show outline when the player is looking at a breakable block (e.g. can't break water or air)
	if (!ChunkSettings::GetBlockData(player.targetBlock).isSolid) return;

	// Enable outline shader and VAO to use correct shaders and buffers
	game.shader.UseShader(Shader::ShaderID::Outline);
	glBindVertexArray(m_outlineVAO);

	// Line indices to determine positions of each line that makes up the outline cube
	static constexpr const std::uint8_t outlineIndices[] = {
		0,  1,  1,  2,  2,  3,  3,  0,	// Right face
		4,  5,  5,  6,  6,  7,  7,  4,	// Left face
		8,  9,  9,  10, 10, 11, 11, 8,	// Top face	
		12, 13, 13, 14, 14, 15, 15, 12,	// Bottom face
		16, 17, 17, 18, 18, 19, 19, 16,	// Front face
		20, 21, 21, 22, 22, 23, 23, 20	// Back face
	};

	// GL_LINES - each pair of indices determine the line's start and end position
	glDrawElements(GL_LINES, sizeof(outlineIndices), GL_UNSIGNED_BYTE, outlineIndices);
}

void Player::RenderPlayerGUI() const noexcept
{
	// Bind VAO and use inventory shader
	game.shader.UseShader(Shader::ShaderID::Inventory);
	glBindVertexArray(m_inventoryVAO);

	// Draw hotbar instances
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, m_noInventoryInstances);
	
	if (player.inventoryOpened) {
		for (int i = 0; i < 9; ++i) world->textRenderer.RenderText(player.inventory[i].slotText, false, !i); // Render hotbar slot text now so they appear under inventory background
		game.shader.UseShader(Shader::ShaderID::Inventory); // Switch back to inventory shader
		glBindVertexArray(m_inventoryVAO); // Use inventory VAO
		glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, m_totalInventoryInstances - m_noInventoryInstances, m_noInventoryInstances); // Draw rest of inventory
	}
}

void Player::SetViewMatrix(glm::mat4& result) const noexcept
{
	result = glm::lookAt(
		player.position,
		player.position + static_cast<glm::dvec3>(m_camFront),
		static_cast<glm::dvec3>(m_camUp)
	);
}

void Player::MouseMoved(double x, double y) noexcept
{
	if (player.inventoryOpened) return;
	float fx = static_cast<float>(x), fy = static_cast<float>(y);

	// Looking straight up causes camera to flip, so stop just before 90 degrees
	const double not90 = 89.9999;

	// Calculate camera angles
	player.yaw = Math::loopAround(player.yaw + (fx * player.sensitivity), -180.0, 180.0);
	player.pitch = std::clamp(player.pitch + (fy * player.sensitivity), -not90, not90);
	player.moved = true; // Update other camera variables

	// Determine camera direction
	player.lookDirectionPitch =
		player.pitch > 60.0f ? WorldDirection::Up :
		player.pitch < -60.0f ? WorldDirection::Down : WorldDirection::None;
	player.lookDirectionYaw =
		(player.yaw >= -45.0f && player.yaw <  45.00f) ? WorldDirection::Front :
		(player.yaw >= 45.00f && player.yaw <  135.0f) ? WorldDirection::Right :
		(player.yaw >= 135.0f || player.yaw < -135.0f) ? WorldDirection::Back  : WorldDirection::Left;

	// Other camera related functions
	UpdateCameraVectors();
	RaycastBlock();
}

void Player::RaycastBlock() noexcept
{
	// Determine steps in each axis
	glm::dvec3 deltaRay = { 
		std::fabs(1.0 / std::fmax(m_camFront.x, 0.01)), 
		std::fabs(1.0 / std::fmax(m_camFront.y, 0.01)), 
		std::fabs(1.0 / std::fmax(m_camFront.z, 0.01)) 
	};

	// Store data about block player was previously looking at
	const WorldPos initialSelectedPosition = player.targetBlockPosition;
	const ObjectID initialSelectedBlock = player.targetBlock;

	// Initial values for raycasting
	player.targetBlockPosition = Math::toWorld(player.position);
	glm::dvec3 sideDistance;
	WorldPos step;

	// Determine directions and magnitude in each axis
	// X axis
	double currentSign = m_camFront.x >= 0.0 ? 1.0 : -1.0;
	step.x = static_cast<PosType>(currentSign);
	sideDistance.x = (player.position.x - static_cast<double>(player.targetBlockPosition.x) + fmax(currentSign, 0.0)) * deltaRay.x * currentSign;
	// Y axis
	currentSign = m_camFront.y >= 0.0 ? 1.0 : -1.0;
	step.y = static_cast<PosType>(currentSign);
	sideDistance.y = (player.position.y - static_cast<double>(player.targetBlockPosition.y) + fmax(currentSign, 0.0)) * deltaRay.y * currentSign;
	// Z axis
	currentSign = m_camFront.z >= 0.0 ? 1.0 : -1.0;
	step.z = static_cast<PosType>(currentSign);
	sideDistance.z = (player.position.z - static_cast<double>(player.targetBlockPosition.z) + fmax(currentSign, 0.0)) * deltaRay.z * currentSign;

	// Determines reach distance
	constexpr double maxDistTravel = 5.5;
	double distanceTravelled = 0.0;
	int maxloops = 5;

	while (distanceTravelled <= maxDistTravel && maxloops--) {
		// Check shortest axis and move in that direction
		enum Axis { X, Y, Z };
		Axis shortestAxis = Axis::X;
		if (sideDistance.y < sideDistance.x) shortestAxis = Axis::Y;
		if (sideDistance.z < sideDistance[static_cast<int>(shortestAxis)]) shortestAxis = Axis::Z;

		switch (shortestAxis)
		{
			case Axis::X:
				player.targetBlockPosition.x += step.x;
				distanceTravelled += sideDistance.x;
				sideDistance.x += deltaRay.x;
				break;
			case Axis::Y:
				player.targetBlockPosition.y += step.y;
				distanceTravelled += sideDistance.y;
				sideDistance.y += deltaRay.y;
				break;
			case Axis::Z:
				player.targetBlockPosition.z += step.z;
				distanceTravelled += sideDistance.z;
				sideDistance.z += deltaRay.z;
				break;
			default:
				break;
		}
		
		// Determine block at current raycast position
		player.targetBlock = world->GetBlock(
			player.targetBlockPosition.x,
			player.targetBlockPosition.y,
			player.targetBlockPosition.z
		);
		
		// Stop if the reached block is a solid block
		if (ChunkSettings::GetBlockData(player.targetBlock).isSolid) break;
	}

	// Set raycast position for future use
	m_rayResult	= player.position + sideDistance;

	// Update block text if needed
	if (player.targetBlock != initialSelectedBlock || player.targetBlockPosition != initialSelectedPosition) UpdateBlockInfoText();
}

void Player::UpdateBlockInfoText() noexcept
{
	const WorldBlockData blockData = ChunkSettings::GetBlockData(player.targetBlock); // Get block properties
	const std::string informationText = fmt::format(
		"Selected:\n({} {} {})\n\"{}\" (ID {})\nLight emission: {}\nReplaceable: {}\nSolid: {}\nTranslucent: {}\nTextures: {} {} {} {} {} {}", 
		player.targetBlockPosition.x, player.targetBlockPosition.y, player.targetBlockPosition.z,
		blockData.name, blockData.id,
		blockData.lightEmission, blockData.natureReplaceable, blockData.isSolid, blockData.hasTransparency,
		blockData.textures[0], blockData.textures[1], blockData.textures[2], blockData.textures[3], blockData.textures[4], blockData.textures[5]
	); // Format block information text
	const float textWidth = world->textRenderer.GetTextScreenWidth(
		informationText, 
		world->textRenderer.GetUnitSizeMultiplier(m_blockText->GetUnitSize())
	); // Get maximum width of block information text
	world->textRenderer.ChangePosition(m_blockText, { 0.99f - textWidth, 0.01f }, false); // Update position to fit all text on screen
	world->textRenderer.ChangeText(m_blockText, informationText); // Update text with block information
}

void Player::UpdateFrustum() noexcept
{
	// Calculate frustum values
	const double halfVside = player.farPlaneDistance * tan(player.fov);
	const double halfHside = halfVside * game.aspect;
	const glm::dvec3 frontMultFar = m_camFront * player.farPlaneDistance;

	// Set each of the frustum planes of the camera
	player.frustum.near = { player.position + (player.nearPlaneDistance * m_camFront), m_camFront };

	glm::dvec3 planeCalc = m_camRight * halfHside;
	player.frustum.right = { player.position, glm::cross(frontMultFar - planeCalc, m_camUp) };
	player.frustum.left = { player.position, glm::cross(m_camUp, frontMultFar + planeCalc) };

	planeCalc = m_camUp * halfVside;
	player.frustum.top = { player.position, glm::cross(m_camRight, frontMultFar - planeCalc) };
	player.frustum.bottom = { player.position, glm::cross(frontMultFar + planeCalc, m_camRight) };
}

void Player::UpdateOffset() noexcept
{
	#ifndef GAME_SINGLE_THREAD
	player.offset = Math::toWorld(player.position * ChunkSettings::CHUNK_SIZE_RECIP_DBL);
	#else
	const WorldPos org = player.offset;
	const glm::dvec3 currentOffset = player.position * ChunkSettings::CHUNK_SIZE_RECIP_DBL;
	player.offset = Math::toWorld(currentOffset);
	if (!game.noGeneration && (org.x != player.offset.x || org.z != player.offset.z)) world->STChunkUpdate();
	#endif
}

void Player::UpdateCameraVectors() noexcept
{
	const double yawRad = glm::radians(player.yaw);
	const double pitchRad = glm::radians(player.pitch);
	const double cosPitchRad = glm::cos(pitchRad);
	
	const glm::vec3 front = glm::normalize(glm::vec3(cos(yawRad) * cosPitchRad, sin(pitchRad), sin(yawRad) * cosPitchRad));
	const glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

	m_camUp = glm::normalize(glm::cross(right, front));
	m_camFront = static_cast<glm::dvec3>(front);
	m_camRight = static_cast<glm::dvec3>(right);

	m_NPcamFront = glm::normalize(glm::vec3(m_camFront.x, 0.0f, m_camFront.z));
	m_NPcamRight = glm::normalize(glm::vec3(m_camRight.x, 0.0f, m_camRight.z));
}

void Player::UpdateScroll(float yoffset) noexcept
{
	const int newSelection = static_cast<int>(selected) + (yoffset > 0.0f ? 1 : -1);
	selected = static_cast<std::uint8_t>(Math::loopAround(newSelection, 0, 9));
	UpdateInventory();
}

void Player::UpdateScroll(int slotIndex) noexcept
{
	selected = static_cast<std::uint8_t>(slotIndex);
	UpdateInventory();
}

void Player::UpdateInventory() noexcept
{
	// Bind VAO and VBO to ensure the correct buffers are edited
	glBindVertexArray(m_inventoryVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_inventoryIVBO);
	
	// Instance of inventory element (item slot, block texture, etc)
	struct InventoryInstance {
		InventoryInstance(
			float x, float y, float w, float h, 
			std::uint32_t texture, bool isBlockTexture = false
		) noexcept : x(x), y(y), w(w), h(h), second(static_cast<int>(isBlockTexture) + (texture << 1)) {}
		InventoryInstance() noexcept {}

		float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
		std::uint32_t second = 0u;
	};

	// Array amount:
	// 5 different rows of 9 slots - 2 hotbars (there is a copy in the inventory gui) and 3 inventory rows
	// Each slot requires 2 instances to also include possible block texture
	// 3 more for crosshair, background and inventory box
	// = 3 + (9 * 5)
	const int maxInstances = 3 + (9 * 5 * 2);
	m_totalInventoryInstances = 0;

	// Each texture index (when data is of an inventory texture rather than a block texture)
	// is used to index into an array that determines the texture positions in the vertex shader.
	enum TextureIndex : std::uint32_t { TI_Background, TI_Unequipped, TI_Equipped, TI_Inventory, TI_Crosshair };
	
	// Inventory slots shader data
	InventoryInstance* inventoryData = new InventoryInstance[maxInstances];

	const float slotWidth = 0.1f, slotHeight = slotWidth * game.aspect;
	const float slotBlockWidth = slotWidth * 0.75f, slotBlockHeight = slotHeight * 0.75f;
	const float slotBlockXOffset = (slotWidth - slotBlockWidth) * 0.5f, slotBlockYOffset = (slotHeight - slotBlockHeight) * 0.5f;
	const float slotsStartX = -0.45f;

	const auto AddHotbarSlot = [&](int hotbarID, TextureIndex texture) noexcept {
		const float hotbarX = slotsStartX + (hotbarID * slotWidth);
		inventoryData[m_totalInventoryInstances++] = InventoryInstance(hotbarX, -1.0f, slotWidth, slotHeight, texture);

		// Also add block texture if there is a valid item in the slot - use top texture
		const std::int32_t itemSlotObject = static_cast<std::uint32_t>(player.inventory[hotbarID].objectID);
		if (itemSlotObject != static_cast<std::int32_t>(ObjectID::Air)) {
			const std::uint32_t blockTexture = ChunkSettings::GetBlockData(itemSlotObject).textures[WorldDirectionInt::IWorldDir_Up];
			inventoryData[m_totalInventoryInstances++] = InventoryInstance(hotbarX + slotBlockXOffset, -1.0f + slotBlockYOffset, slotBlockWidth, slotBlockHeight, blockTexture, true);
		}
	};

	// Hotbar slots calculation
	for (int hotbarID = 0; hotbarID < 9; ++hotbarID) {
		if (hotbarID == selected) continue;
		AddHotbarSlot(hotbarID, TI_Unequipped);
	}

	AddHotbarSlot(selected, TI_Equipped); // Make selected slot render on top of other slots
	const int hotbarSlotsInstances = m_totalInventoryInstances;

	inventoryData[m_totalInventoryInstances++] = InventoryInstance(-0.025f, -0.025f, 0.025f / game.aspect, 0.025f, TI_Crosshair); // Crosshair at center of screen (always visible)
	m_noInventoryInstances = m_totalInventoryInstances; // Determines what instances are rendered when inventory is not opened
	
	// Inventory slots
	const float inventoryHotbarYPosition = -0.4f;
	const float inventoryYPosition = inventoryHotbarYPosition + (slotHeight * 1.1f);
	
	inventoryData[m_totalInventoryInstances++] = InventoryInstance(-1.00f, -1.00f, 2.0f, 2.0f, TI_Background); // Translucent background for inventory GUI (full screen)
	inventoryData[m_totalInventoryInstances++] = InventoryInstance(-0.475f, -0.43f, 0.0f, 0.0f, TI_Inventory); // Inventory GUI background
	
	// Copy hotbar into inventory, with a different Y position
	for (int i = 0; i < hotbarSlotsInstances; ++i) {
		InventoryInstance hotbarInstance = inventoryData[i];
		hotbarInstance.y = hotbarInstance.second & 1u ? inventoryHotbarYPosition + slotBlockYOffset : inventoryHotbarYPosition; // 1st bit determines if this instance is a block texture
		inventoryData[m_totalInventoryInstances++] = hotbarInstance;
	}

	// Hotbar slots are stored in the first 9 indexes in player inventory
	for (int inventoryID = 9; inventoryID < 36; ++inventoryID) {
		const int inventoryColumnIndex = inventoryID % 9, inventoryRowIndex = (inventoryID - 9) / 9;
		const float slotX = slotsStartX + (inventoryColumnIndex * slotWidth), slotY = inventoryYPosition + (inventoryRowIndex * slotHeight);
		inventoryData[m_totalInventoryInstances++] = InventoryInstance(slotX, slotY, slotWidth, slotHeight, TI_Unequipped);

		// Same as hotbar, check for valid slot item
		const std::uint32_t itemSlotObject = static_cast<std::uint32_t>(player.inventory[inventoryID].objectID);
		if (itemSlotObject != static_cast<std::uint32_t>(ObjectID::Air)) {
			inventoryData[m_totalInventoryInstances++] = InventoryInstance(slotX + slotBlockXOffset, slotY + slotBlockYOffset, slotBlockWidth, slotBlockHeight, itemSlotObject, true);
		}
	}

	// Buffer inventory data for use in the shader - only include the filled data rather than the whole array
	glBufferData(GL_ARRAY_BUFFER, sizeof(InventoryInstance) * m_totalInventoryInstances, inventoryData, GL_STATIC_DRAW);
}

Player::~Player() noexcept
{
	// List of all buffers from outline and inventory to be deleted
	const GLuint deleteBuffers[] = {
		static_cast<GLuint>(m_outlineVBO),
		static_cast<GLuint>(m_inventoryIVBO),
		static_cast<GLuint>(m_inventoryQuadVBO),
	};

	// Delete the buffers from the above array
	glDeleteBuffers(sizeof(deleteBuffers) / sizeof(GLuint), deleteBuffers);

	// Delete both VAOs
	const GLuint deleteVAOs[] = { m_outlineVAO, m_inventoryVAO };
	glDeleteVertexArrays(sizeof(deleteVAOs) / sizeof(GLuint), deleteVAOs);
}
