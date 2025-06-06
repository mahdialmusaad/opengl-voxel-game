#pragma once
#ifndef _SOURCE_PLAYER_PLR_HDR_
#define _SOURCE_PLAYER_PLR_HDR_

#include "World/World.hpp"

// definition NOT visible to world
class Player
{
private:
	GLuint m_outlineVAO, m_inventoryVAO, m_inventoryIVBO, m_inventoryQuadVBO, m_outlineVBO;
	std::uint8_t m_noInventoryInstances, m_totalInventoryInstances;
	std::uint8_t selected = 0;
	TextRenderer::ScreenText *m_blockText;
public:
	PlayerObject player;
	World *world;

	Player() noexcept;
	void InitializeText() noexcept;
	void CheckInput() noexcept;

	void InventoryTextTest() noexcept;

	void UpdateMovement() noexcept;
	void SetPosition(glm::dvec3 newPos) noexcept;
	const glm::dvec3& GetVelocity() const noexcept;

	void BreakBlock() noexcept;
	ModifyWorldResult PlaceBlock() noexcept;

	void MouseMoved(double x, double y) noexcept;
	void ProcessKeyboard(WorldDirection move) noexcept;

	int SearchForItem(ObjectID item) noexcept;
	int SearchForItemNonFull(ObjectID item) noexcept;

	void RenderBlockOutline() const noexcept;
	void RenderPlayerGUI() const noexcept;
	void SetViewMatrix(glm::mat4 &result) const noexcept;

	void UpdateOffset() noexcept;
	void UpdateFrustum() noexcept;

	void UpdateScroll(float yoffset) noexcept;
	void UpdateScroll(int slotIndex) noexcept;
	void UpdateInventory() noexcept;

	~Player() noexcept;
private:
	void UpdatePositionalVariables() noexcept;
	void UpdateCameraVectors() noexcept;
	void RaycastBlock() noexcept;

	void UpdateBlockInfoText() noexcept;
	glm::dvec3 m_camUp{};
	glm::dvec3 m_camFront{};
	glm::dvec3 m_camRight{};

	glm::dvec3 m_NPcamFront{};
	glm::dvec3 m_NPcamRight{};

	glm::dvec3 m_rayResult{};
	glm::dvec3 m_velocity{};
};

#endif
