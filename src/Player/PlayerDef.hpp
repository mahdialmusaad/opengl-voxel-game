#pragma once
#ifndef _SOURCE_PLAYER_PLRDEF_HDR_
#define _SOURCE_PLAYER_PLRDEF_HDR_

#include "World/Chunk.hpp"
#include "Rendering/TextRenderer.hpp"

// Player variables that are able to be accessed by the world
struct PlayerObject
{
	struct InventorySlot 
	{
		ObjectID objectID = ObjectID::Air;
		std::uint8_t count = 0;
		TextRenderer::ScreenText *slotText;
	};

	bool moved = true;
	bool noclip = false;
	bool grounded = true;
	bool doGravity = false;
	bool inventoryOpened = false;

	WorldDirection lookDirectionPitch = WldDir_None, lookDirectionYaw = WldDir_None;

	ObjectID targetBlock = ObjectID::Air;
	ObjectID headBlock = ObjectID::Air;
	ObjectID feetBlock = ObjectID::Air;

	glm::dvec3 position = {};

	double posMagnitude = 0.0;
	WorldPos offset{};
	CameraFrustum frustum;

	InventorySlot inventory[36];
	WorldPos targetBlockPosition {};

	double fov = glm::radians(90.0);
	double pitch = 0.0f, yaw = 0.0f;

	double currentSpeed = 2.0, defaultSpeed = 2.0;

	static const double gravity;
	static const double waterGravityMultiplier;

	static const double maxGravity;
	static const double jumpPower;

	static const double reachDistance;
	static const std::uint8_t maxSlotCount;

	static const double farPlaneDistance;
	static const double nearPlaneDistance;
};

#endif // _SOURCE_PLAYER_PLRDEF_HDR_
