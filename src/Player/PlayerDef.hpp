#pragma once
#ifndef _SOURCE_PLAYER_PLRDEF_HDR_
#define _SOURCE_PLAYER_PLRDEF_HDR_

#include "World/Chunk.hpp"
#include "Rendering/TextRenderer.hpp"

// definition visible to world
enum class ModifyWorldResult {
	Passed,
	AboveWorld,
	BelowWorld,
	NotFound,
	Invalid
};

struct PlayerObject
{
	struct InventorySlot 
	{
		ObjectID objectID = ObjectID::Air;
		std::uint8_t count = 0;
		TextRenderer::ScreenText* slotText;
	};

	bool moved = true;
	bool doGravity = false;
	bool inventoryOpened = false;
	WorldDirection lookDirectionPitch, lookDirectionYaw = WorldDirection::Front;

	ObjectID targetBlock = ObjectID::Air;

	glm::dvec3 position = {};

	double posMagnitude = 0.0;
	WorldPos offset{};
	CameraFrustum frustum;

	InventorySlot inventory[36];
	WorldPos targetBlockPosition {};

	double fov = glm::radians(90.0);
	double sensitivity = 0.1f;
	double pitch = 0.0f, yaw = 0.0f;

	static constexpr const char* directionText[7] = { "None", "East", "West", "Up", "Down", "North", "South" };

	double currentSpeed = 2.0, defaultSpeed = 2.0;

	static constexpr double gravity = -3.0;
	static constexpr double maxGravity = -100.0;
	static constexpr double jumpPower = 0.5;

	static constexpr std::uint8_t maxSlotCount = 99u;

	static constexpr double farPlaneDistance = 10000.0;
	static constexpr double nearPlaneDistance = 0.1;
};

#endif // _SOURCE_PLAYER_PLRDEF_HDR_
