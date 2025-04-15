#pragma once
#ifndef _SOURCE_UTILITY_APPLICATION_HDR_
#define _SOURCE_UTILITY_APPLICATION_HDR_

#include <Player/Player.hpp>
#include <World/Sky.hpp>

class Badcraft
{
public:
	struct Callbacks
	{
		Callbacks();
		Badcraft* app = nullptr;

		// The window is already defined, so no need for the first argument 
		// (usually GLFWwindow*)

		void KeyPressCallback(int key, int scancode, int action, int mods);
		void MouseClickCallback(int button, int action, int) noexcept;
		void MouseMoveCallback(double xpos, double ypos) noexcept;
		void ResizeCallback(int _width, int _height) noexcept;
		void ScrollCallback(double, double yoffset) noexcept;
		void WindowMoveCallback(int x, int y) noexcept;
		void CharCallback(unsigned codepoint);
		void CloseCallback() noexcept;

		void ToggleInventory() noexcept;
		void BeginChat() noexcept;
		void ApplyChat();

		typedef std::function<void()> func;
		typedef std::pair<int, func> pair;

		static constexpr int press = GLFW_PRESS;
		static constexpr int rept = GLFW_PRESS | GLFW_REPEAT;
		
		const std::unordered_map<int, pair> keyFunctionMap = {
			// Single-press inputs
			{ GLFW_KEY_ESCAPE,	pair(press, func([&]() { game.mainLoopActive = false; }))},
			{ GLFW_KEY_E,		pair(press, func([&]() { ToggleInventory(); }))},
			{ GLFW_KEY_Z,		pair(press, func([&]() { glPolygonMode(GL_FRONT_AND_BACK, b_not(app->wireframe) ? GL_LINE : GL_FILL); }))},
			{ GLFW_KEY_X,		pair(press, func([&]() { glfwSwapInterval(!b_not(app->maxFPS)); }))},
			{ GLFW_KEY_C,		pair(press, func([&]() { b_not(app->player.collisionEnabled); }))},
			{ GLFW_KEY_V,		pair(press, func([&]() { b_not(game.test); }))},
			{ GLFW_KEY_R,		pair(press, func([&]() { game.shader.InitShader(); app->UpdateAspect(); }))},
			{ GLFW_KEY_F1,		pair(press, func([&]() { b_not(app->showGUI); }))},
			{ GLFW_KEY_F2,		pair(press, func([&]() { app->TakeScreenshot(); }))},
			{ GLFW_KEY_F3,		pair(press, func([&]() { glfwSetInputMode(game.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); game.focusChanged = true; }))},
			{ GLFW_KEY_SLASH,	pair(press, func([&]() { app->showGUI = true; BeginChat(); }))},
			// Repeatable inputs
			{ GLFW_KEY_COMMA,	pair(rept, func([&]() { app->player.currentSpeed += 1.0f; app->player.defaultSpeed = app->player.currentSpeed; }))},
			{ GLFW_KEY_PERIOD,	pair(rept, func([&]() { app->player.currentSpeed -= 1.0f; app->player.defaultSpeed = app->player.currentSpeed; }))},
			{ GLFW_KEY_O,		pair(rept, func([&]() { app->player.fov += Math::TORADIANS_FLT; app->UpdatePerspective(); }))},
			{ GLFW_KEY_I,		pair(rept, func([&]() { app->player.fov -= Math::TORADIANS_FLT; app->UpdatePerspective(); }))},
		};
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

#endif
