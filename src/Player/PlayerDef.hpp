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
	};

	bool moved = true;
	bool collisionEnabled = false;
	bool inventoryOpened = false;
	WorldDirection lookDirectionPitch = WorldDirection::None, lookDirectionYaw = WorldDirection::None;

	ObjectID targetBlock = ObjectID::Air;

	glm::dvec3 position = { 
		ChunkSettings::CHUNK_SIZE_DBL / 2.0, 
		static_cast<double>(fmin(ChunkSettings::MAX_WORLD_HEIGHT, 64)),
		ChunkSettings::CHUNK_SIZE_DBL / 2.0
	};

	double posMagnitude = 0.0;
	WorldPos offset{};
	CameraFrustum frustum;

	InventorySlot inventory[36];
	WorldPos targetBlockPosition {};

	double fov = 1.8;
	double sensitivity = 0.1f;
	double pitch = 0.0f, yaw = 0.0f;

	double currentSpeed = 2.0, defaultSpeed = 2.0;

	static constexpr double gravity = -3.0;
	static constexpr double maxGravity = -100.0;

	static constexpr double farPlaneDistance = 10000.0;
	static constexpr double nearPlaneDistance = 0.1;
};

#endif // _SOURCE_PLAYER_PLRDEF_HDR_
