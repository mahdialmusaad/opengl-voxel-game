#include "Player.hpp"

Player::Player() noexcept
{
	TextFormat::log("Player enter");

	// Temporary inventory test
	for (PlayerObject::InventorySlot& s : player.inventory) s.objectID = static_cast<ObjectID>((rand() % 8) + 1);

	// Setup VAO and VBO for selection outline
	m_outlineVAO = OGL::CreateVAO8();
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
	m_inventoryVAO = OGL::CreateVAO8();

	// Enable attributes 0-2: 0 = vec4, 1 = int, 2 = base vec3
	glEnableVertexAttribArray(0u);
	glEnableVertexAttribArray(1u);
	glEnableVertexAttribArray(2u);

	// Attribute divisors for instanced rendering
	glVertexAttribDivisor(0u, 1u);
	glVertexAttribDivisor(1u, 1u);

	// VBO for the inventory texture data
	m_inventoryIVBO = OGL::CreateBuffer8(GL_ARRAY_BUFFER);

	// Attribute layout (location 0 and 1)
	// A0 (vec4): X, Y, W, H
	// A1 (int): Texture type - text or block (1st bit), inner texture id (other bits)
	constexpr GLsizei stride = sizeof(float[4]) + sizeof(std::int32_t);
	glVertexAttribPointer(0u, 4, GL_FLOAT, GL_FALSE, stride, nullptr);
	glVertexAttribIPointer(1u, 1, GL_INT, stride, reinterpret_cast<const void*>(sizeof(float[4])));

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
	//m_blockText = world->textRenderer.CreateText(glm::vec2(0.0f, 0.0f), "");

	// Inventory item counters
	//inventoryCountText = new TextRenderer::ScreenText*[36];
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
		if (!player.collisionEnabled) {
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
	m_velocity.x = Math::lerp(m_velocity.x, 0.0, game.deltaTime * 5.0);
	m_velocity.y = Math::lerp(m_velocity.y, 0.0, game.deltaTime * 5.0);
	m_velocity.z = Math::lerp(m_velocity.z, 0.0, game.deltaTime * 5.0);

	// Check if the player velocity is close enough to zero to simply treat it as not moving
	// (Smooth player movement means the player could move by a millionth of a block, which is basically 
	// nothing/unnoticeable but would still require expensive collision checks and other calculations)
	constexpr double movementMinimum = 0.001;
	if (fabs(m_velocity.x) + fabs(m_velocity.y) + fabs(m_velocity.z) <= movementMinimum) return;

	if (player.collisionEnabled) {
		// Gravity should have a maximum instead of increasing indefinetly 
		m_velocity.y = fmax(m_velocity.y + (player.gravity * game.deltaTime), player.maxGravity);

		/*
			For each direction (per axis):

			Get the block found at the new position added with the relative position,
			(checking for 0.1 blocks in each direction for more accurate collisions)

			Check if the found block is solid,

			and remove velocity in that direction if their signs match
			(e.g. don't remove positive X velocity if a block was found in negative X direction)
		*/

		struct CollisionInfo {
			constexpr CollisionInfo(glm::dvec3 p, int axis, bool pos) noexcept :
				direction(p), axis(axis), positiveAxis(pos) {
			}
			const glm::dvec3 direction;
			const int axis;
			const bool positiveAxis;
		};

		// Lists of all relative block coordinates to check for collision, optimized to check 
		// the most common scenarios first and seperated by axis to skip uneccessary checks
		// e.g. if a player collided with a block on their right, there is no need to check
		// for collisions on the left and vice versa

		// X axis collision checks
		constexpr const CollisionInfo XCollisionChecks[4] = {
			{ {  0.1, -0.1,  0.0 }, 0, true  },	// On the right at feet height
			{ { -0.1, -0.1,  0.0 }, 0, false },	// On the left at feet height
			{ {  0.1,  0.0,  0.0 }, 0, true  },	// On the right at head height
			{ { -0.1,  0.0,  0.0 }, 0, false },	// On the left at head height
		};

		// Y axis collision checks
		constexpr const CollisionInfo YCollisionChecks[2] = {
			{ { 0.0, -0.1,  0.0 }, 1, false },	// Below feet
			{ { 0.0,  0.1,  0.0 }, 1, true  },	// Above head
		};

		// Z axis collision checks
		constexpr const CollisionInfo ZCollisionChecks[4] = {
			{ { 0.0, -0.1,  0.1 }, 2, false },	// In front at feet height
			{ { 0.0, -0.1, -0.1 }, 2, false },	// Behind at feet height
			{ { 0.0,  0.0,  0.1 }, 2, true  },	// In front at head height
			{ { 0.0,  0.0, -0.1 }, 2, true  },	// Behind at head height
		};

		// The new position to check collisions for
		const glm::dvec3 newPosition = player.position + m_velocity;

		// Collision checks in each axis:
		// X axis
		for (const CollisionInfo& colInfo : XCollisionChecks) {
			const WorldPos pos = newPosition + colInfo.direction;
			if (ChunkSettings::GetBlockData(world->GetBlock(pos.x, pos.y, pos.z)).isSolid) {
				double& axis = m_velocity[colInfo.axis];
				if ((axis >= 0.0) == colInfo.positiveAxis) { axis = 0.0; break; }
			}
		}
		// Y axis
		for (const CollisionInfo& colInfo : YCollisionChecks) {
			const WorldPos pos = newPosition + colInfo.direction;
			if (ChunkSettings::GetBlockData(world->GetBlock(pos.x, pos.y, pos.z)).isSolid) {
				double& axis = m_velocity[colInfo.axis];
				if ((axis >= 0.0) == colInfo.positiveAxis) { axis = 0.0; break; }
			}
		}
		// Z axis
		for (const CollisionInfo& colInfo : ZCollisionChecks) {
			const WorldPos pos = newPosition + colInfo.direction;
			if (ChunkSettings::GetBlockData(world->GetBlock(pos.x, pos.y, pos.z)).isSolid) {
				double& axis = m_velocity[colInfo.axis];
				if ((axis >= 0.0) == colInfo.positiveAxis) { axis = 0.0; break; }
			}
		}
	}
	else player.position += m_velocity;

	// Update position-related variables
	UpdatePositionalVariables();
}

void Player::SetPosition(glm::dvec3 newPos) noexcept
{
	// Set the new position of the player
	player.position = newPos;
	UpdatePositionalVariables();
}

void Player::UpdatePositionalVariables() noexcept
{
	player.moved = true;
	UpdateOffset();
	RaycastBlock();
}

void Player::BreakBlock() noexcept
{
	if (ChunkSettings::GetBlockData(player.targetBlock).isSolid) {
		world->SetBlock(player.targetBlockPosition.x, player.targetBlockPosition.y, player.targetBlockPosition.z, ObjectID::Air);
		RaycastBlock();
	}
}

ModifyWorldResult Player::PlaceBlock() noexcept
{
	// Determine selected block in inventory
	const ObjectID placeBlock = player.inventory[selected].objectID;

	// Only place if the current selected block is valid
	if (placeBlock != ObjectID::Air && ChunkSettings::GetBlockData(player.targetBlock).isSolid) {
		const ModifyWorldResult result = world->SetBlock(
			player.targetBlockPosition.x, 
			player.targetBlockPosition.y,
			player.targetBlockPosition.z,
			placeBlock
		);

		RaycastBlock();
		return result;
	}

	return ModifyWorldResult::Invalid;
}

void Player::ProcessKeyboard(WorldDirection direction) noexcept
{
	const double directionMultiplier = player.currentSpeed * game.deltaTime;

	// Add to velocity based on frame time and input
	switch (direction) {
		// Player movement
		case WorldDirection::Front:
			m_velocity += m_camFront * directionMultiplier;
			break;
		case WorldDirection::Back:
			m_velocity += -m_camFront * directionMultiplier;
			break;
		case WorldDirection::Left:
			m_velocity += -m_camRight * directionMultiplier;
			break;
		case WorldDirection::Right:
			m_velocity += m_camRight * directionMultiplier;
			break;
		// Flying movement
		case WorldDirection::Up:
			m_velocity += m_camUp * directionMultiplier;
			break;
		case WorldDirection::Down:
			m_velocity += -m_camUp * directionMultiplier;
			break;
		// Jumping logic
		case WorldDirection::None:
			// Lazy 'grounded' check
			if (abs(m_velocity.y) < 0.1) break;
			m_velocity += glm::dvec3(0.0, 5.0, 0.0);
			break;
		default:
			break;
	}
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

	// Only draw the inventory if it is opened
	const int instances = static_cast<int>(player.inventoryOpened ? m_totalInventoryInstances : m_hotbarInstances);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, instances);
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
	constexpr double not90 = 89.9999;

	// Calculate camera angles
	player.yaw = Math::loopAround(player.yaw + (fx * player.sensitivity), -180.0, 180.0);
	player.pitch = Math::clamp(player.pitch + (fy * player.sensitivity), -not90, not90);
	player.moved = true; // Update other camera variables

	// Determine camera direction (unused for now)
	player.lookDirectionPitch =
		player.pitch > 60.0f ? WorldDirection::Up :
		player.pitch < -60.0f ? WorldDirection::Down : WorldDirection::None;
	player.lookDirectionYaw =
		player.yaw >= -45.0f && player.yaw < 45.0f ? WorldDirection::Front :
		player.yaw >= 45.0f && player.yaw < 135.0f ? WorldDirection::Right :
		player.yaw >= 135.0f || player.yaw < -135.0f ? WorldDirection::Back : WorldDirection::Left;

	// Other camera related functions
	UpdateCameraVectors();
	RaycastBlock();
}

void Player::RaycastBlock() noexcept
{
	// Determine steps in each axis
	glm::dvec3 deltaRay = { 
		fabs(1.0 / fmax(m_camFront.x, 0.01)), 
		fabs(1.0 / fmax(m_camFront.y, 0.01)), 
		fabs(1.0 / fmax(m_camFront.z, 0.01)) 
	};

	// Initial values for raycasting
	player.targetBlockPosition = Math::toWorld(player.position);
	glm::dvec3 sideDistance;
	WorldPos step;

	// Determine directions and magnitude in each axis
	// X axis
	double currentSign = m_camFront.x >= 0.0 ? 1.0 : -1.0;
	step.x = currentSign;
	sideDistance.x = (player.position.x - static_cast<double>(player.targetBlockPosition.x) + fmax(currentSign, 0.0)) * deltaRay.x * currentSign;
	// Y axis
	currentSign = m_camFront.y >= 0.0 ? 1.0 : -1.0;
	step.y = currentSign;
	sideDistance.y = (player.position.y - static_cast<double>(player.targetBlockPosition.y) + fmax(currentSign, 0.0)) * deltaRay.y * currentSign;
	// Z axis
	currentSign = m_camFront.z >= 0.0 ? 1.0 : -1.0;
	step.z = currentSign;
	sideDistance.z = (player.position.z - static_cast<double>(player.targetBlockPosition.z) + fmax(currentSign, 0.0)) * deltaRay.z * currentSign;

	// Determines reach distance
	constexpr double maxDistTravel = 5.5;
	double distanceTravelled = 0.0;
	int maxloops = 5;

	//const ObjectID initialSelectedBlock = player.targetBlock;

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
	/*if (initialSelectedBlock != player.targetBlock) {
		const WorldBlockData blockData = ChunkSettings::GetBlockData(player.targetBlock); // Get block properties
		const std::string nameText = blockData.name;
		// Update position to fit all text on screen
		world->textRenderer.ChangePosition(m_blockText, { 1.0f - (0.01f * nameText.size()), 0.18f }, false);
		// Update text with block information
		world->textRenderer.ChangeText(m_blockText, fmt::format("{} ({})", 5, blockData.id));
	}*/
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
	const double pitchRadians = glm::radians(player.pitch);
	const double yawRadians = glm::radians(player.yaw);
	const double cosPitchRadians = cosf(pitchRadians);

	m_camFront = glm::normalize(glm::vec3(
		cos(yawRadians) * cosPitchRadians,
		sin(pitchRadians),
		sin(yawRadians) * cosPitchRadians
	));

	constexpr glm::dvec3 vecUp = glm::dvec3(0.0, 1.0, 0.0);
	m_camRight = glm::normalize(glm::cross(m_camFront, vecUp));
	m_camUp = glm::normalize(glm::cross(m_camRight, m_camFront));
}

void Player::UpdateScroll(float yoffset) noexcept
{
	const int newSelection = static_cast<int>(selected) + (yoffset > 0.0f ? 1 : -1);
	selected = static_cast<std::uint8_t>(Math::loopAroundInteger(newSelection, 0, 9));
	UpdateInventory();
}

void Player::UpdateInventory() noexcept
{
	// Bind VAO and VBO to ensure the correct buffers are edited
	glBindVertexArray(m_inventoryVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_inventoryIVBO);
	
	// Instance of inventory element (item slot, block texture, etc)
	struct InventoryInstance {
		constexpr InventoryInstance(
			float x, float y, float w, float h, 
			std::int32_t texture, bool isBlockTexture = false
		) noexcept : x(x), y(y), w(w), h(h), second(static_cast<int>(isBlockTexture) + (texture << 1)) {}
		constexpr InventoryInstance() noexcept {}

		float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
		std::int32_t second = 0;
	};

	// Array amount: 2 uints per instance (InventoryInstance struct)
	// 5 different rows of 9 slots (2 hotbars (1 is a copy in inventory gui) and 3 inventory slots)
	// Each slot requires 2 instances to also include possible block texture
	// 3 more for crosshair, background and inventory box
	// = 3 + (2 * 9 * 5)
	constexpr int maxInstances = 3 + (9 * 5 * 2);
	m_totalInventoryInstances = 0;

	// Each texture index (when data is of an inventory texture rather than a block texture)
	// is used to index into an array that determines the texture positions in the vertex shader.
	enum TextureIndex : std::uint32_t { TI_Background, TI_Unequipped, TI_Equipped, TI_Inventory, TI_Crosshair };
	
	// Compressed buffer data
	InventoryInstance* data = new InventoryInstance[maxInstances];
	data[m_totalInventoryInstances++] = InventoryInstance(-0.025f, -0.025f, 0.025f, 0.025f * game.aspect, TI_Crosshair); // Crosshair at center of screen (always visible)

	// Positions and sizes
	constexpr float 
		inventorySlotsYPosition = -0.36f,
		slotsXPosition = -0.4f,
		slotsXSpacing = 0.094f,
		inventorySlotSize = 0.1f,
		inventoryHotbarRowYOffset = 0.06f,
		blocksPositionOffset = 0.008f,
		blocksSlotSize = 0.083f;

	for (int inventoryID = 0; inventoryID < 45; ++inventoryID) {
		int iid = inventoryID;
		if (inventoryID == 9) {
			// Calculating inventory now rather than hotbar (first 9)
			m_hotbarInstances = m_totalInventoryInstances;
			// Add inventory GUI and background to data
			const InventoryInstance inventoryData[2] = {
				InventoryInstance(-1.0f, -1.0f, 2.0f, 2.0f, TI_Background),		// Translucent background for inventory GUI (full screen)
				InventoryInstance(-0.475f, -0.43f, 1.0f, 0.8f, TI_Inventory)	// Inventory GUI background
			};
			std::memcpy(data + m_totalInventoryInstances, inventoryData, sizeof(inventoryData));
			m_totalInventoryInstances += 2;
		}

		// The hotbar slots need to be displayed again in the inventory, so redo the first row
		if (inventoryID >= 9) iid = inventoryID - 9;

		/*
			Calculate suitable X and Y position for each slot:
			(There are 9 slots per 'row', hotbar = bottom 9 slots, inventory hotbar = hotbar but shown in inventory GUI)

			Hotbar row is located at the bottom of the screen
			The hotbar row is also visible in the inventory at the bottom of the inventory GUI
			The inner inventory slots have equal spacing each row, with slightly more after the hotbar row
		*/

		const float GUIYPosition = inventoryID >= 9 ? inventorySlotsYPosition + // Y pos. spacing between rows in inventory
			(inventoryID >= 18 ? inventoryHotbarRowYOffset : 0.0f) + // Inventory hotbar row extra spacing
			((inventorySlotSize * game.aspect) * static_cast<float>(iid / 9)) // Equal spacing between inventory rows
			: -1.0f; // Hotbar slots are at the bottom
		const float GUIXPosition = slotsXPosition + (slotsXSpacing * static_cast<float>(iid % 9));

		// Set compressed hotbar data with either unequipped or equipped slot texture
		data[m_totalInventoryInstances++] = InventoryInstance(
			GUIXPosition, GUIYPosition, inventorySlotSize, inventorySlotSize * game.aspect, 
			(iid == selected ? TI_Equipped : TI_Unequipped)
		);

		// Add the corresponding texture if the current inventory slot is not empty
		ObjectID block = player.inventory[iid].objectID;
		if (block != ObjectID::Air) {
			// Get the texture ID of the top texture of the current block in the inventory
			const std::uint32_t texID = static_cast<std::uint32_t>(ChunkSettings::GetBlockTexture(block, WorldDirection::Up));
			data[m_totalInventoryInstances++] = InventoryInstance(
				GUIXPosition + blocksPositionOffset, GUIYPosition + (blocksPositionOffset * 2.0f), 
				blocksSlotSize, blocksSlotSize * game.aspect, texID, true
			);
		}
	}

	// Buffer inventory data for use in the shader - only include the filled data rather than the whole array
	const GLsizeiptr totalInstances = static_cast<GLsizeiptr>(m_totalInventoryInstances);
	glBufferData(GL_ARRAY_BUFFER, totalInstances * static_cast<GLsizeiptr>(sizeof(InventoryInstance)), data, GL_STATIC_DRAW);
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
