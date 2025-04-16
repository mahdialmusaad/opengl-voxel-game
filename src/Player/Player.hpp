#pragma once
#ifndef _SOURCE_PLAYER_PLR_HDR_
#define _SOURCE_PLAYER_PLR_HDR_

#include "World/World.hpp"

// definition NOT visible to world
class Player
{
private:
	uint8_t m_outlineVAO, m_inventoryVAO, m_inventoryIVBO;
	uint8_t m_hotbarInstances, m_totalInventoryInstances;
	uint8_t selected = 0;
	TextRenderer::ScreenText** inventoryCountText;
public:
	PlayerObject player;
	World *world;

	Player() noexcept;
	void UpdateMovement() noexcept;
	void SetPosition(glm::dvec3 newPos) noexcept;

	void BreakBlock() noexcept;
	ModifyWorldResult PlaceBlock() noexcept;

	void MouseMoved(double x, double y) noexcept;
	void ProcessKeyboard(WorldDirection move) noexcept;

	void RenderBlockOutline() const noexcept;
	void RenderPlayerGUI() const noexcept;
	void SetViewMatrix(glm::mat4 &result) const noexcept;

	void UpdateOffset() noexcept;
	void UpdateFrustum() noexcept;

	void UpdateScroll(float yoffset) noexcept;
	void UpdateInventory() noexcept;
private:
	void UpdatePositionalVariables() noexcept;
	void UpdateCameraVectors() noexcept;
	void RaycastBlock() noexcept;

	glm::vec3 m_camUp{};
	glm::vec3 m_camFront{};
	glm::vec3 m_camRight{};

	glm::dvec3 m_rayResult{};
	glm::dvec3 m_velocity{};
};

#endif
