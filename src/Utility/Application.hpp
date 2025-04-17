#pragma once
#ifndef _SOURCE_UTILITY_APPLICATION_HDR_
#define _SOURCE_UTILITY_APPLICATION_HDR_

#include "Player/Player.hpp"
#include "World/Sky.hpp"

class Badcraft
{
public:
	struct Callbacks
	{
		Callbacks();
		Badcraft* app = nullptr;

		void KeyPressCallback(int key, int scancode, int action, int mods);
		void MouseClickCallback(int button, int action, int) noexcept;
		void MouseMoveCallback(double xpos, double ypos) noexcept;
		void ResizeCallback(int _width, int _height) noexcept;
		void ScrollCallback(double, double yoffset) noexcept;
		void WindowMoveCallback(int x, int y) noexcept;
		void CharCallback(unsigned codepoint);
		void CloseCallback() noexcept;

		void ToggleInventory() noexcept;
        void ApplyInput(int key, int action) noexcept;
		void BeginChat() noexcept;
		void ApplyChat();
	};

	Callbacks callbacks;
	Player playerFunctions;
	PlayerObject& player;
	World world;

	bool showGUI = true;
	bool maxFPS = false;
	bool wireframe = false;

	Badcraft();
	void Main();

	void UpdateAspect();
	void UpdatePerspective();
private:
	void TakeScreenshot();

	void TextUpdate();
	void ExitGame();

	void UpdateFrameValues();

	enum MatrixIndexes : int { Perspective, View, World, Origin, };
	glm::mat4 m_matrices[4];

	WorldSky m_skybox;
	uint8_t m_matricesUBO, m_timesUBO, m_coloursUBO, m_positionsUBO, m_sizesUBO;

	double m_lastFrameTime = 0.0, m_updateTime = 0.0;
	int m_nowFPS, m_avgFPS, m_lowFPS;

	TextRenderer::ScreenText *m_infoText, *m_debugText, *m_screenshotText, *m_chatText;
};

#endif // _SOURCE_UTILITY_APPLICATION_HDR_
