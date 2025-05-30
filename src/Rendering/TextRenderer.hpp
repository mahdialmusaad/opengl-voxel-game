#pragma once
#ifndef _SOURCE_RENDERING_TEXTRENDERER_HDR_
#define _SOURCE_RENDERING_TEXTRENDERER_HDR_

#include "Globals/Definitions.hpp"

class TextRenderer
{
public:
	typedef glm::vec<4, std::uint8_t> ColourData;

	enum TextSettings: std::uint8_t {
		TS_Background = 1u,
		TS_BackgroundFullX = 2u,
		TS_BackgroundFullY = 4u,
		TS_Shadow = 8u
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
		
		std::string& GetText() noexcept;
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

		std::uint16_t m_displayLength = 0u;
		std::uint8_t m_unitSize = 16u;

		std::uint8_t m_settings; 

		/* 	Settings: 
			Bit 0: Background
			Bit 1: Background full X
			Bit 2: Background full Y
			Bit 3: Shadow
		*/

		ColourData m_RGBColour = { 255u, 255u, 255u, 255u };
		GLuint m_vbo;
	public:
		TextType textType;
		~ScreenText() noexcept;
	};

	static constexpr float hideTimer = 5.0f;
	static constexpr float defaultUnitSize = 12.0f;
	static constexpr float spaceCharacterSize = 0.015f;
	static constexpr int maxChatCharacterWidth = 40;
	static constexpr int maxChatLines = 12;
	
	float characterSpacingUnits = 0.0f;
	float textWidth = 0.0f;
	float textHeight = 0.0f;

	TextRenderer() noexcept;
	void RenderText() const noexcept;

	ScreenText* CreateText(
		glm::vec2 pos, 
		std::string text, 
		std::uint8_t settings = 0u,
		TextType textType = TextType::Default,
		std::uint8_t unitSize = static_cast<std::uint8_t>(defaultUnitSize)
	) noexcept;
	void RecalculateAllText() noexcept;

	ScreenText* GetTextFromID(std::uint16_t id) noexcept;
	std::uint16_t GetIDFromText(ScreenText* screenText) const noexcept;

	void ChangeText(ScreenText* screenText, std::string newText, bool update = true) noexcept;
	void ChangePosition(ScreenText* screenText, glm::vec2 newPos, bool update = true) noexcept;
	void ChangeUnitSize(ScreenText* screenText, std::uint8_t newUnitSize, bool update = true) noexcept;
	void ChangeColour(ScreenText* screenText, ColourData newColour, bool update = true) noexcept;
	void ChangeSettings(ScreenText* screenText, std::uint8_t settigns, bool update = true) noexcept;

	float GetCharScreenWidth(int charIndex, float unitMultiplier) const noexcept;

	void CheckTextStatus() noexcept;
	void RemoveText(std::uint16_t id) noexcept;

	~TextRenderer() noexcept;
private:
	// Characters sizes as seen in text texture (follows ASCII, starting after the 'space' character)
	static constexpr std::uint8_t m_charSizes[95] = {
	//  !  "  #  $  %  &  '  (  )  *  +  ,  -  .  /
		1, 3, 6, 5, 9, 6, 1, 2, 2, 5, 5, 2, 5, 1, 3,
	//  0  1  2  3  4  5  6  7  8  9
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	//  :  ;  <  =  >  ?  @ 
		1, 2, 4, 5, 4, 5, 6,
	//  A  B  C  D  E  F  G  H  I  J  K  L  M
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	//  N  O  P  Q  R  S  T  U  V  W  X  Y  Z
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	//  [  \  ]  ^  _  `
		2, 3, 2, 5, 5, 2,
	//  a  b  c  d  e  f  g  h  i  j  k  l  m
		4, 4, 4, 4, 4, 4, 4, 4, 1, 3, 4, 3, 5,
	//  n  o  p  q  r  s  t  u  v  w  x  y  z
		4, 4, 4, 4, 4, 4, 3, 4, 5, 5, 5, 4, 4,
	//  {  |  }  ~
		3, 1, 3, 6,
	// 	Background character
		1
	};

	std::unordered_map<std::uint16_t, ScreenText*> m_screenTexts;
	std::uint8_t texturePositionsLocation;
	std::uint8_t m_textVAO, m_textVBO;

	void UpdateText(ScreenText* screenText) const noexcept;
	std::uint16_t GetNewID() noexcept;
};

#endif // _TEXTRENDERER_HEADER_
