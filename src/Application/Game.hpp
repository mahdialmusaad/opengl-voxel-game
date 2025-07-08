#pragma once
#ifndef _SOURCE_APPLICATION_GAME_HDR_
#define _SOURCE_APPLICATION_GAME_HDR_

#include "Player/Player.hpp"
#include "World/Sky.hpp"

class GameObject
{
public:
	struct Callbacks
	{
		Callbacks(GameObject *appPtr);
	
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
		void AddCommandText(std::string newText) noexcept;
		void ApplyCommand();
	private:
		GameObject *m_app = nullptr;
		std::vector<std::string> m_previousChats;
		int m_chatHistorySelector = -1;

		std::vector<std::string> CMD_args;
		int CMD_currentIndex = 0;
		
		bool HasArgument(int index) { return static_cast<int>(CMD_args.size()) > index; }
		std::string &GetArgument(int index) { CMD_currentIndex = index + 1; return CMD_args[index]; }
		template<typename T> T IntArg(int index) { return static_cast<T>(TextFormat::strtoi64(GetArgument(index))); }
		template<typename T> T IntArg(int index, T min, T max) { return glm::clamp(static_cast<T>(TextFormat::strtoi64(GetArgument(index))), min, max);	}
		double DblArg(int index, double min = std::numeric_limits<double>::lowest(), double max = std::numeric_limits<double>::max()) {
			return glm::clamp(std::stod(GetArgument(index)), min, max);
		}
		
		typedef std::vector<std::pair<int, std::string>> ConversionList;
		void DoConvert(const ConversionList &argsConversions);
		void ConvertArgument(std::string &arg, const std::string &actual);
	};

	Player playerFunctions;
	PlayerObject &player;
	World world;

	GameObject() noexcept;
	void Main();

	void UpdateAspect() noexcept;
	void UpdatePerspective() noexcept;

	~GameObject();
private:
	void DebugFunctionTest() noexcept;

	void UpdateFrameValues() noexcept;
	void MoveMatricesUpdate() noexcept;
	void GameMiscUpdate() noexcept;

	void ExitGame() noexcept;
	
	enum MatrixIndexes : int { Matrix_Perspective, Matrix_View, Matrix_World, Matrix_Origin };
	glm::mat4 m_matrices[4];
	
	Skybox m_skybox;
	GLuint m_matricesUBO, m_timesUBO, m_coloursUBO, m_positionsUBO, m_sizesUBO;
	
	double m_lastTime = 0.0, m_updateTime = 0.0;
	int m_nowFPS, m_avgFPS, m_lowFPS;
	
	TextRenderer::ScreenText *m_infoText, *m_infoText2, *m_chatText, *m_commandText;

	double EditTime(bool isSet, double t = 0.0) noexcept;
	float GetRelativeTextPosition(TextRenderer::ScreenText *sct, int linesOverride = -1) noexcept;
public:
	Callbacks callbacks;
};

#endif // _SOURCE_APPLICATION_GAME_HDR_
