#pragma once
#ifndef _SOURCE_PLAYER_PLR_HDR_
#define _SOURCE_PLAYER_PLR_HDR_

#include "World/World.hpp"

// Full player class including 'player object' for world class
class Player
{
private:
	GLuint m_outlineVAO, m_inventoryVAO, m_raycastVAO;
	GLuint m_inventoryIVBO, m_inventoryQuadVBO, m_outlineVBO, m_raycastVBO;
	GLuint m_outlineEBO;

	std::uint8_t m_noInventoryInstances, m_totalInventoryInstances;
	std::uint8_t selected = 0;

	glm::i8vec3 placeBlockRelPosition;

	TextRenderer::ScreenText *m_blockText;
	TextRenderer::ScreenText *m_inventoryHotbarText[9];

	struct InventoryInstance {
		InventoryInstance(
			glm::vec4 dims, std::uint32_t texture, bool isBlockTexture = false
		) noexcept : dims(dims), second(static_cast<int>(isBlockTexture) + (texture << 1)) {}
		InventoryInstance() noexcept {}

		glm::vec4 dims{};
		std::uint32_t second = 0u;
	} *inventoryData = new InventoryInstance[3 + (9 * 5 * 2)];
public:
	PlayerObject player;
	World *world;

	enum class PlayerDirection : int { Forwards, Backwards, Left, Right, Fly_Up, Fly_Down, Jump };

	Player() noexcept;
	void WorldInitialize() noexcept;
	void CheckInput() noexcept;

	void InventoryTextTest() noexcept;
	void RaycastDebugCheck() noexcept;

	void ApplyMovement() noexcept;
	void SetPosition(const glm::dvec3 &newPos) noexcept;
	const glm::dvec3 &GetVelocity() const noexcept;

	void BreakBlock() noexcept;
	void PlaceBlock() noexcept;

	void UpdateCameraDirection(double x = 0.0, double y = 0.0) noexcept;
	void AddVelocity(PlayerDirection move) noexcept;

	int SearchForItem(ObjectID item, bool includeFull) noexcept;
	int SearchForFreeMatchingSlot(ObjectID item) noexcept;

	void RenderBlockOutline() const noexcept;
	void RenderPlayerGUI() const noexcept;
	void SetViewMatrix(glm::mat4 &result) const noexcept;

	void UpdateOffset() noexcept;
	void UpdateFrustum() noexcept;

	void UpdateScroll(float yoffset) noexcept;
	void UpdateScroll(int slotIndex) noexcept;

	void UpdateInventory() noexcept;
	void UpdateInventoryTextPos() noexcept;
	void UpdateSlot(int slotIndex, ObjectID object, std::uint8_t count);
	
	void UpdateBlockInfoText() noexcept;

	~Player() noexcept;
private:
	void PositionVariables() noexcept;
	void UpdateCameraVectors() noexcept;
	void RaycastBlock() noexcept;

	enum class SlotType : int { Hotbar, InvHotbar, Inventory, SlotSize, SlotBlockSize };

	glm::vec2 GetSlotPos(SlotType stype, int id, bool isBlock) const noexcept;
	glm::vec4 GetSlotDims(SlotType stype, int id, bool isBlock) const noexcept;

	glm::dvec3 m_camUp{};
	glm::dvec3 m_camFront{};
	glm::dvec3 m_camRight{};

	glm::dvec3 m_NPcamFront{};
	glm::dvec3 m_NPcamRight{};
	
	glm::dvec3 m_velocity{};
};

#endif
