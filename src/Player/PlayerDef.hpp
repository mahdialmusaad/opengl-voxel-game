#pragma once
#ifndef _SOURCE_PLAYER_PLRDEF_HDR_
#define _SOURCE_PLAYER_PLRDEF_HDR_

#include "World/Chunk.hpp"
#include "Rendering/TextRenderer.hpp"

// Player variables that are able to be accessed by the world
struct WorldPlayer
{
	struct InventorySlot 
	{
		ObjectID objectID = ObjectID::Air;
		std::uint8_t count{};
		TextRenderer::ScreenText *slotText;
	};

	bool noclip = false;
	bool doGravity = false;
	bool inventoryOpened = false;

	bool moved = true;
	bool grounded = true;

	WorldDirection lookDirectionPitch = WldDir_None, lookDirectionYaw = WldDir_None;

	ObjectID targetBlock = ObjectID::Air;
	ObjectID headBlock = ObjectID::Air;
	ObjectID feetBlock = ObjectID::Air;

	glm::dvec3 position{};

	//double posMagnitude;
	WorldPosition offset{};
	CameraFrustum frustum{};

	InventorySlot inventory[36];
	WorldPosition targetBlockPosition{};

	double fov = glm::radians(90.0);
	double pitch = 0.0, yaw = 0.0;

	double defaultSpeed = 1.0, currentSpeed = defaultSpeed;
};

#endif // _SOURCE_PLAYER_PLRDEF_HDR_
