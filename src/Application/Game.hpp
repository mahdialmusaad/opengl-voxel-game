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
		void ToggleFullscreen() noexcept;
		void ApplyInput(int key, int action) noexcept;
		void AddChatMessage(std::string message, bool autonl = true) noexcept;
		void BeginChat() noexcept;
		void AddCommandText(std::string newText) noexcept;
		void ApplyCommand();
	private:
		struct ConversionData {
			ConversionData(int i, bool b, const std::string &s) noexcept : index(i), decimal(b), strarg(s) {}
			int index;
			bool decimal;
			std::string strarg;
		};

		bool revBool(bool &b) const noexcept { b = !b; return b; }

		GameObject *m_app = nullptr;
		std::vector<std::string> m_previousChats;
		int m_chatHistorySelector = -1;

		std::vector<std::string> CMD_args;
		int currentCMDInd = 0;

		ConversionData cvd = { 0, false, "" };
		bool queryChat = true;
		
		bool HasArgument(int index) { return static_cast<int>(CMD_args.size()) > index; }
		std::string &GetArg(int index) { currentCMDInd = index; return CMD_args[index]; }
		
		template<typename T, typename C = T> C IntArg(int index, T min = std::numeric_limits<T>::lowest(), T max = std::numeric_limits<T>::max()) {
			return static_cast<C>(glm::clamp(static_cast<T>(TextFormat::strtoimax(GetArg(index))), min, max));
		}
		double DblArg(int index, double min = std::numeric_limits<double>::lowest(), double max = std::numeric_limits<double>::max());

		void CMDConv(const std::vector<ConversionData> &argsConversions);
		void CMDConv(const ConversionData &data);

		template<typename T> bool isdec(const T&) const noexcept { return std::numeric_limits<T>::is_iec559; } 
		template<typename T> ConversionData tcv(int index, T val) const noexcept { return { index, isdec(val), fmt::to_string(val) }; }

		template<typename T> void query(const std::string &name, T value) noexcept {
			const std::string asStr = fmt::format(std::is_integral<T>::value ? "{}" : "{:.3f}", value);
			if (queryChat) AddChatMessage(fmt::format("Current {} is {}", name, asStr));
			cvd = ConversionData(0, isdec(value), asStr);
		}

		template<glm::length_t L, typename T, glm::qualifier Q> void queryMult(
			const std::string &name,
			const glm::vec<L, T, Q> &value,
			int startIndex = 0
		) noexcept {
			std::string fmtstr;
			for (int i = 0, m = L - 1; i < L; ++i) fmtstr += fmt::format("{:.3f}{}", value[i], i != m ? " " : "");
			if (queryChat) AddChatMessage(fmt::format("Current {}: {}", name, fmtstr));
			cvd = ConversionData(startIndex, isdec(value[0]), fmtstr);
		}
	};

	Player playerFunctions;
	WorldPlayer &player;
	World world;

	GameObject() noexcept;
	static void InitGame(char *location);
	void Main();

	void UpdateAspect() noexcept;
private:
	void DebugFunctionTest() noexcept;
	void PerlinResultTest() const noexcept;

	void UpdateFrameValues() noexcept;
	void MovedUpdate() noexcept;
	void MiscUpdate() noexcept;
	
	glm::mat4 m_perspectiveMatrix;
	
	Skybox m_skybox;
	
	double m_lastTime = 0.0, m_updateTime = 0.0;
	int m_nowFPS, m_avgFPS, m_lowFPS;
	
	TextRenderer::ScreenText *m_infoText, *m_infoText2, *m_chatText, *m_commandText, *m_perfText;

	double EditTime(bool isSet, double t = 0.0) noexcept;
public:
	// Ensure other variables have been initialized first (inner struct accesses them)
	Callbacks callbacks;
};

#endif // _SOURCE_APPLICATION_GAME_HDR_
