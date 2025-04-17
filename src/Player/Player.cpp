#include "Player.hpp"

Player::Player() noexcept
{
	TextFormat::log("Player enter");

	// temporary inventory test
	for (PlayerObject::InventorySlot& s : player.inventory) s.objectID = narrow_cast<ObjectID>((rand() % 8) + 1);
	//inventoryCountText = new TextRenderer::ScreenText*[36];

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
	OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeCoordinates), cubeCoordinates, GL_STATIC_DRAW);
	glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, sizeof(float[3]), nullptr);

	// Setup VAO for inventory
	m_inventoryVAO = OGL::CreateVAO8();
	glEnableVertexAttribArray(0u);
	glEnableVertexAttribArray(1u);
	glEnableVertexAttribArray(2u);
	glVertexAttribDivisor(2u, 1u);

	struct QuadVertex {
		constexpr QuadVertex(float a, float b, float c) noexcept :
			a(a), b(b), c(c), i(static_cast<int32_t>(a)), j(static_cast<int32_t>(b)) {}
		float a, b, c;
		int32_t i, j;
	};

	constexpr const QuadVertex quadVerticesData[4] = {
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f },
		{ 1.0f, 1.0f, 0.0f },
		{ 1.0f, 0.0f, 1.0f }
	};

	// VBO for the quad vertices defined above (location 0 and 1)
	OGL::CreateBuffer(GL_ARRAY_BUFFER);
	glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
	glVertexAttribIPointer(1u, 2, GL_INT, 5 * sizeof(float), reinterpret_cast<const void*>(3 * sizeof(float)));
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(quadVerticesData), quadVerticesData, 0u);

	// VBO for the actual inventory texture data
	m_inventoryIVBO = OGL::CreateBuffer8(GL_ARRAY_BUFFER);
	UpdateInventory();

	UpdateCameraVectors();
	UpdateFrustum();
	UpdateOffset();

	TextFormat::log("Player exit");
}

void Player::UpdateMovement() noexcept
{
	// Player should slowly come to a stop instead of speed being directly related to keyboard input

	// TODO: Framerate-independent lerp/smoothing
	m_velocity.x = Math::lerp(m_velocity.x, 0.0, game.deltaTime);
	m_velocity.y = Math::lerp(m_velocity.y, 0.0, game.deltaTime);
	m_velocity.z = Math::lerp(m_velocity.z, 0.0, game.deltaTime);

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
		constexpr const CollisionInfo X_collisionPositions[4] = {
			{ {  0.1, -0.1,  0.0 }, 0, true  },	// On the right at feet height
			{ { -0.1, -0.1,  0.0 }, 0, false },	// On the left at feet height
			{ {  0.1,  0.0,  0.0 }, 0, true  },	// On the right at head height
			{ { -0.1,  0.0,  0.0 }, 0, false },	// On the left at head height
		};

		// Y axis collision checks
		constexpr const CollisionInfo Y_collisionPositions[2] = {
			{ { 0.0, -0.1,  0.0 }, 1, false },	// Below feet
			{ { 0.0,  0.1,  0.0 }, 1, true  },	// Above head
		};

		// Z axis collision checks
		constexpr const CollisionInfo Z_collisionPositions[4] = {
			{ { 0.0, -0.1,  0.1 }, 2, false },	// In front at feet height
			{ { 0.0, -0.1, -0.1 }, 2, false },	// Behind at feet height
			{ { 0.0,  0.0,  0.1 }, 2, true  },	// In front at head height
			{ { 0.0,  0.0, -0.1 }, 2, true  },	// Behind at head height
		};

		// The new position to check collisions for
		const glm::dvec3 newPosition = player.position + m_velocity;

		// Collision checks in each axis (order doesn't matter - same results)
		for (const CollisionInfo& colInfo : X_collisionPositions) {
			const WorldPos pos = newPosition + colInfo.direction;
			if (ChunkSettings::GetBlockData(world->GetBlock(pos.x, pos.y, pos.z)).isSolid) {
				double& axis = m_velocity[colInfo.axis];
				if ((axis >= 0.0) == colInfo.positiveAxis) { axis = 0.0; break; }
			}
		}

		for (const CollisionInfo& colInfo : Y_collisionPositions) {
			const WorldPos pos = newPosition + colInfo.direction;
			if (ChunkSettings::GetBlockData(world->GetBlock(pos.x, pos.y, pos.z)).isSolid) {
				double& axis = m_velocity[colInfo.axis];
				if ((axis >= 0.0) == colInfo.positiveAxis) { axis = 0.0; break; }
			}
		}

		for (const CollisionInfo& colInfo : Z_collisionPositions) {
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
	const ObjectID placeBlock = player.inventory[selected].objectID;
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
	const float directionMultiplier = static_cast<float>(player.currentSpeed * game.deltaTime);

	switch (direction) {
		case WorldDirection::Front:
			m_velocity += static_cast<glm::dvec3>(m_camFront * directionMultiplier);
			break;
		case WorldDirection::Back:
			m_velocity += static_cast<glm::dvec3>(-m_camFront * directionMultiplier);
			break;
		case WorldDirection::Left:
			m_velocity += static_cast<glm::dvec3>(-m_camRight * directionMultiplier);
			break;
		case WorldDirection::Right:
			m_velocity += static_cast<glm::dvec3>(m_camRight * directionMultiplier);
			break;
		case WorldDirection::Up:
			// TODO: jump direction variable
			m_velocity += glm::dvec3(0.0, 10.0, 0.0);
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
	static constexpr const uint8_t outlineIndices[] = {
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
	game.shader.UseShader(Shader::ShaderID::Inventory);
	glBindVertexArray(m_inventoryVAO);
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

	constexpr float not90 = 90.0f - 1e-05f;
	player.yaw = Math::loopAround(player.yaw + (fx * player.sensitivity), -180.0f, 180.0f);
	player.pitch = std::clamp(player.pitch + (fy * player.sensitivity), -not90, not90);
	player.moved = true;

	player.lookDirectionPitch =
		player.pitch > 60.0f ? WorldDirection::Up :
		player.pitch < -60.0f ? WorldDirection::Down : WorldDirection::None;

	player.lookDirectionYaw =
		player.yaw >= -45.0f && player.yaw < 45.0f ? WorldDirection::Front :
		player.yaw >= 45.0f && player.yaw < 135.0f ? WorldDirection::Right :
		player.yaw >= 135.0f || player.yaw < -135.0f ? WorldDirection::Back : WorldDirection::Left;

	UpdateCameraVectors();
	RaycastBlock();
}

void Player::RaycastBlock() noexcept
{
	glm::vec3 rayDirection = m_camFront;

	glm::vec3 rayUnitStep = {
		sqrt(1.0f + (rayDirection.y / rayDirection.x) * (rayDirection.y / rayDirection.x)),
		sqrt(1.0f + (rayDirection.x / rayDirection.y) * (rayDirection.x / rayDirection.y)),
		sqrt(1.0f + (rayDirection.z / rayDirection.y) * (rayDirection.z / rayDirection.y))
	};

	m_rayResult = player.position;
	player.targetBlockPosition = m_rayResult;
	glm::vec3 rayLength{};
	WorldPos step{};

	// Determine the amount and direction to move in each axis

	// X axis
	if (rayDirection.x < 0.0f) {
		step.x = -onePosType;
		rayLength.x = static_cast<float>(m_rayResult.x - static_cast<double>(player.targetBlockPosition.x)) * rayUnitStep.x;
	}
	else {
		step.x = onePosType;
		rayLength.x = static_cast<float>(static_cast<double>(player.targetBlockPosition.x + onePosType) - m_rayResult.x) * rayUnitStep.x;
	}

	// Y axis
	if (rayDirection.y < 0.0f) {
		step.y = -onePosType;
		rayLength.y = static_cast<float>(m_rayResult.y - static_cast<double>(player.targetBlockPosition.y)) * rayUnitStep.y;
	}
	else {
		step.y = onePosType;
		rayLength.y = static_cast<float>(static_cast<double>(player.targetBlockPosition.y + onePosType) - m_rayResult.y) * rayUnitStep.y;
	}

	// Z axis
	if (rayDirection.z < 0.0f) {
		step.z = -onePosType;
		rayLength.z = static_cast<float>(m_rayResult.z - static_cast<double>(player.targetBlockPosition.z)) * rayUnitStep.z;
	}
	else {
		step.z = onePosType;
		rayLength.z = static_cast<float>(static_cast<double>(player.targetBlockPosition.z + onePosType) - m_rayResult.z) * rayUnitStep.z;
	}

	float distanceTravelled = 0.0f;
	enum class Axis : int { X, Y, Z };
	int max = 20;

	while (distanceTravelled < player.reachDistance && --max) {
		// Check shortest axis and move in that direction
		Axis shortestAxis = Axis::X;
		if (rayLength.y < rayLength.x) shortestAxis = Axis::Y;
		//if (rayLength.z < rayLength.y) shortestAxis = Axis::Z;

		switch (shortestAxis)
		{
			case Axis::X:
				player.targetBlockPosition.x += step.x;
				distanceTravelled += rayLength.x;
				rayLength.x += rayUnitStep.x;
				break;
			case Axis::Y:
				player.targetBlockPosition.y += step.y;
				distanceTravelled += rayLength.y;
				rayLength.y += rayUnitStep.y;
				break;
				/*case Axis::Z:
					player.targetBlockPosition.z += step.z;
					distanceTravelled += rayLength.z;
					rayLength.z += rayUnitStep.z;
					break;*/
			default:
				break;
		}

		// Check if the reached block is a solid block (not air, water, etc)
		m_rayResult = m_rayResult + static_cast<glm::dvec3>(rayDirection * distanceTravelled);
		player.targetBlockPosition = Math::toWorld(m_rayResult);

		player.targetBlock = world->GetBlock(
			player.targetBlockPosition.x,
			player.targetBlockPosition.y,
			player.targetBlockPosition.z
		);
	}
}

void Player::UpdateFrustum() noexcept
{
	// Calculate frustum values
	const float halfVside = player.farPlaneDistance * tanf(player.fov * 0.6f);
	const float halfHside = halfVside * game.aspect;
	const glm::vec3 frontMultFar = m_camFront * player.farPlaneDistance;

	// Set each of the frustum planes of the camera
	glm::vec3 p_pos = static_cast<glm::vec3>(player.position);
	player.frustum.near = { p_pos + (player.nearPlaneDistance * m_camFront), m_camFront };

	glm::vec3 planeCalc = m_camRight * halfHside;
	player.frustum.right = { p_pos, glm::cross(frontMultFar - planeCalc, m_camUp) };
	player.frustum.left = { p_pos, glm::cross(m_camUp, frontMultFar + planeCalc) };

	planeCalc = m_camUp * halfVside;
	player.frustum.top = { p_pos, glm::cross(m_camRight, frontMultFar - planeCalc) };
	player.frustum.bottom = { p_pos, glm::cross(frontMultFar + planeCalc, m_camRight) };
}

void Player::UpdateOffset() noexcept
{
	#ifndef BADCRAFT_1THREAD
	player.offset = Math::toWorld(player.position * ChunkSettings::CHUNK_SIZE_RECIP_DBL);
	#else
	const WorldPos org = player.offset;
	player.offset = Math::toWorld(player.position * ChunkSettings::CHUNK_SIZE_RECIP_DBL);
	if (org.x != player.offset.x || org.z != player.offset.z) world->TestChunkUpdate();
	#endif
}

void Player::UpdateCameraVectors() noexcept
{
	const float pitchRadians = Math::TORADIANS_FLT * player.pitch;
	const float yawRadians = Math::TORADIANS_FLT * player.yaw;
	const float cosPitchRadians = cosf(pitchRadians);

	m_camFront = glm::normalize(glm::vec3(
		cosf(yawRadians) * cosPitchRadians,
		sinf(pitchRadians),
		sinf(yawRadians) * cosPitchRadians)
	);

	constexpr const glm::vec3 vecUp = glm::vec3(0.0f, 1.0f, 0.0f);
	m_camRight = glm::normalize(glm::cross(m_camFront, vecUp));
	m_camUp = glm::normalize(glm::cross(m_camRight, m_camFront));
}

void Player::UpdateInventory() noexcept
{
	// Bind VAO and VBO to ensure the correct buffers are edited
	glBindVertexArray(m_inventoryVAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_inventoryIVBO);
	glVertexAttribIPointer(2u, 2, GL_UNSIGNED_INT, 0, nullptr);

	enum TextureIndex : uint32_t { Background, Unequipped, Equipped, Inventory, Crosshair };
	// Last bit in second uint32 is used to determine if current instance uses the blocks or inventory texture
	constexpr uint32_t blockBit = 1 << 31;
	struct InventoryInstance {
		constexpr InventoryInstance(uint32_t xy = 0u, uint32_t tw = 0u, bool block = false) noexcept :
			xy(xy), tw(tw + (block ? blockBit : 0u)) {
		}
		uint32_t xy, tw;
	};

	// Array amount: 2 uints per instance (InventoryInstance struct)
	// 5 different rows of 9 slots (2 hotbars - in inventory and bottom and 3 inventory slots)
	// Each slot requires 2 instances (4 uints) to also include block texture
	// 3 more for crosshair, background and inventory box
	// = 3 + (2 * 9 * 5)
	constexpr int numInstances = 3 + (9 * 5 * 2);

	// Compressed buffer data
	InventoryInstance data[numInstances] = {
		InventoryInstance(0u, Crosshair) // Crosshair at center of screen (always visible)
	};
	m_totalInventoryInstances = 1; // Ensure counter reflects this ^^^

	// Bits used to store X and Y position in first uint32
	constexpr uint32_t positionShift = 16u;
	// Float multiplier for integer compression
	constexpr float shaderMultiplier = static_cast<float>((1u << positionShift) - 1u),
		inventorySlotsYPosition = 0.18f,
		inventorySlotsYSpacing = 0.073f,
		inventoryXSpacing = 0.0495f;

	for (int inventoryID = 0; inventoryID < 45; ++inventoryID) {
		int iid = inventoryID;
		if (inventoryID == 9) {
			// Calculating inventory now rather than hotbar (first 9)
			m_hotbarInstances = m_totalInventoryInstances;
			// Add inventory GUI and background to data
			constexpr const InventoryInstance inventoryData[2] = {
				InventoryInstance(0u, Background),	// Translucent background for inventory GUI
				InventoryInstance(0u, Inventory)	// Inventory GUI background
			};
			memcpy(data + m_totalInventoryInstances, inventoryData, sizeof(inventoryData));
			m_totalInventoryInstances += 2;
		}

		// The hotbar slots need to be displayed again in the inventory, so redo the first row
		if (inventoryID >= 9) iid = inventoryID - 9;
		const float slotRowIndex = static_cast<float>(iid % 9);

		/*
			Calculate suitable X and Y position for each slot :

			(There are 9 slots per row)
			Hotbar row is located at the bottom of the screen
			The hotbar row is also visible in the inventory at the bottom of the inventory GUI
			The inner inventory slots have equal spacing each row, with slightly more after the hotbar row
		*/

		const float slotYPosition =
			inventoryID >= 9 ? inventorySlotsYPosition + // Spaces between rows in inventory
			(inventoryID >= 18 ? 0.01f : 0.0f) + // Inventory hotbar row extra spacing
			(inventorySlotsYSpacing * static_cast<float>(iid / 9)) // Equal spacing between inventory rows
			: 0.0f;	// Hotbar slots are at the bottom
		// Calculate screen position in shader integer value (X and Y position take 16 bits)
		const uint32_t screenPos =
			static_cast<uint32_t>(inventoryXSpacing * slotRowIndex * shaderMultiplier) +
			(static_cast<uint32_t>(slotYPosition * shaderMultiplier) << positionShift);

		// Set compressed hotbar data with either unequipped or equipped slot texture
		data[m_totalInventoryInstances++] = InventoryInstance(
			screenPos,
			(iid == selected ? Equipped : Unequipped)
		);

		// Add the corresponding texture if the current inventory slot is not empty
		ObjectID block = player.inventory[iid].objectID;
		if (block != ObjectID::Air) {
			// Get the texture ID of the top texture of the current block in the inventory
			const uint32_t texID = static_cast<uint32_t>(ChunkSettings::GetBlockTexture(block, WorldDirection::Up));
			data[m_totalInventoryInstances++] = InventoryInstance(screenPos, texID, true);
		}
	}

	// Buffer inventory data for use in the shader - only include the filled data rather than the whole array
	size_t totalInstances = static_cast<size_t>(m_totalInventoryInstances);
	glBufferData(GL_ARRAY_BUFFER, totalInstances * sizeof(InventoryInstance), data, GL_STATIC_DRAW);
}

void Player::UpdateScroll(float yoffset) noexcept
{
	int newSelection = static_cast<int>(selected) + (yoffset > 0.0f ? 1 : -1);
	selected = narrow_cast<uint8_t>(Math::loopAroundInteger(newSelection, 0, 9));
	UpdateInventory();
}
