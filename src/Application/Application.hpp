#pragma once
#ifndef _SOURCE_UTILITY_APPLICATION_HDR_
#define _SOURCE_UTILITY_APPLICATION_HDR_

#include "Player/Player.hpp"
#include "World/Sky.hpp"

class GameObject
{
public:
	struct Callbacks
	{
		Callbacks();
		GameObject* app = nullptr;
		std::string previousChat;

		void KeyPressCallback(int key, int scancode, int action, int mods);
		void MouseClickCallback(int button, int action, int) noexcept;
		void MouseMoveCallback(double xpos, double ypos) noexcept;
		void ResizeCallback(int _width, int _height) noexcept;
		void ScrollCallback(double, double yoffset) noexcept;
		void WindowMoveCallback(int x, int y) noexcept;
		void CharCallback(unsigned codepoint);
		void CloseCallback() noexcept;

		void TakeScreenshot() noexcept;
		void ToggleInventory() noexcept;
		void ApplyInput(int key, int action) noexcept;
		void AddChatMessage(std::string message) noexcept;
		void BeginChat() noexcept;
		void ApplyCommand();
	};

	Callbacks callbacks;
	Player playerFunctions;
	PlayerObject& player;
	World world;

	GameObject();
	void Main();

	void UpdateAspect();
	void UpdatePerspective();

	~GameObject();
private:
	void ExitGame();

	void UpdateFrameValues();
	void MovementUpdate();
	void TextUpdate();

	enum MatrixIndexes : int { Matrix_Perspective, Matrix_View, Matrix_World, Matrix_Origin, };
	glm::mat4 m_matrices[4];

	Skybox m_skybox;
	std::uint8_t m_matricesUBO, m_timesUBO, m_coloursUBO, m_positionsUBO, m_sizesUBO;

	double m_lastTime = 0.0, m_updateTime = 0.0;
	int m_nowFPS, m_avgFPS, m_lowFPS;

	TextRenderer::ScreenText *m_infoText, *m_debugText, *m_chatText, *m_commandText, *m_blockText;
};

#endif // _SOURCE_UTILITY_APPLICATION_HDR_
