#pragma once
#ifndef _SOURCE_RENDERING_TEXTRENDERER_HDR_
#define _SOURCE_RENDERING_TEXTRENDERER_HDR_

#include "Application/Definitions.hpp"

class TextRenderer
{
public:
	typedef glm::vec<4, std::uint8_t> ColourData;

	enum TextSettings : std::uint8_t {
		TS_Background = static_cast<std::uint8_t>(1u),
		TS_BackgroundFullX = static_cast<std::uint8_t>(2u),
		TS_BackgroundFullY = static_cast<std::uint8_t>(4u),
		TS_Shadow = static_cast<std::uint8_t>(8u),
		TS_InventoryVisible = static_cast<std::uint8_t>(16u),
		TS_DebugText = static_cast<std::uint8_t>(32u)
	};

	enum class TextType : std::uint8_t {
		Default,
		Temporary,
		TemporaryShow,
		Hidden
	};

	struct ScreenText
	{
		ScreenText(glm::vec2 pos, std::string text, TextType type, std::uint8_t settings, std::uint8_t unitSize) noexcept;
		GLuint GetVBO() const noexcept;
		void ResetTextTime() noexcept;
		
		void _ChangeInternalText(std::string newText) noexcept;
		void _ChangeInternalPosition(glm::vec2 newPos) noexcept;
		void _ChangeInternalColour(ColourData newColour) noexcept;
		void _ChangeInternalSettings(std::uint8_t settings) noexcept;
		void _ChangeInternalUnitSize(std::uint8_t unitSize) noexcept;
		
		const std::string &GetText() const noexcept;
		ColourData GetColour() const noexcept;
		glm::vec2 GetPosition() const noexcept;
		std::uint8_t GetUnitSize() const noexcept;
		std::uint8_t GetSettings() const noexcept;
		
		float GetTime() const noexcept;
		int GetDisplayLength() const noexcept;
		int HasSettingEnabled(TextSettings setting) const noexcept;
	private:
		std::string m_text;
		glm::vec2 m_pos = { 0.0f, 0.0f };
		float m_textTime = 0.0f;

		std::uint16_t m_displayLength{};
		std::uint8_t m_settings{};
		std::uint8_t m_unitSize = static_cast<std::uint8_t>(16u);

		ColourData m_RGBColour = ColourData(static_cast<std::uint8_t>(255u));
		GLuint m_vbo;
	public:
		TextType textType;
		~ScreenText() noexcept;
	};

	const float defTextWidth = 0.006f;
	
	float spaceCharacterSize = 0.04f, characterSpacingSize = 0.009f, textWidth = defTextWidth, textHeight = 0.06f;
	int maxChatLineChars = 60, maxChatLines = 11;

	static constexpr unsigned defaultUnitSize = 10u;

	TextRenderer() noexcept;
	void UpdateShaderUniform() noexcept;

	void RenderText(bool inventoryStatus) const noexcept;
	void RenderText(ScreenText *screenText, bool shader, bool inventoryStatus) const noexcept;

	ScreenText *CreateText(
		glm::vec2 pos, 
		std::string text, 
		unsigned settings = 0,
		unsigned unitSize = defaultUnitSize,
		TextType textType = TextType::Default,
		bool update = true
	) noexcept;
	ScreenText *CreateText(ScreenText *original, bool update = true);

	void RecalculateAllText() noexcept;

	ScreenText *GetTextFromID(std::uint16_t id) noexcept;
	std::uint16_t GetIDFromText(ScreenText *screenText) const noexcept;

	void ChangeText(ScreenText *screenText, std::string newText, bool update = true) noexcept;
	void ChangePosition(ScreenText *screenText, glm::vec2 newPos, bool update = true) noexcept;
	void ChangeUnitSize(ScreenText *screenText, std::uint8_t newUnitSize, bool update = true) noexcept;
	void ChangeColour(ScreenText *screenText, ColourData newColour, bool update = true) noexcept;
	void ChangeSettings(ScreenText *screenText, std::uint8_t settings, bool update = true) noexcept;

	float GetUnitSizeMultiplier(std::uint8_t unitSize) const noexcept;
	float GetCharScreenWidth(int charIndex, float unitMultiplier) const noexcept;
	float GetTextWidth(const std::string &text, std::uint8_t unit) const noexcept;

	float GetTextHeight(ScreenText *screenText) const noexcept;
	float GetTextHeight(std::uint8_t fontSize, int lines) const noexcept;
	float GetRelativeTextYPos(TextRenderer::ScreenText *sct, int linesOverride = -1) noexcept;

	void FormatChatText(std::string &text) const noexcept;

	void CheckTextStatus() noexcept;
	void RemoveText(std::uint16_t id) noexcept;

	~TextRenderer() noexcept;
private:
	std::unordered_map<std::uint16_t, ScreenText*> m_screenTexts;
	GLint texturePositionsLocation;
	std::uint16_t m_textVAO, m_textVBO;

	void UpdateText(ScreenText *screenText) const noexcept;
	std::uint16_t GetNewID() noexcept;
};

#endif // _TEXTRENDERER_HEADER_
