#pragma once
#ifndef _SOURCE_RENDERING_TEXTRENDERER_HDR_
#define _SOURCE_RENDERING_TEXTRENDERER_HDR_

#include "Globals/Definitions.hpp"

class TextRenderer
{
public:
	struct ScreenText
	{
		enum class TextType : std::uint8_t {
			Default = 0, 
			Temporary = 1, 
			TemporaryShow = 2,
			Hidden = 3
		};

		typedef glm::vec<4, std::uint8_t> ColourData;
		
		ScreenText(glm::vec2 pos, std::string text, TextType type, std::uint8_t fontSize) noexcept;

		void ResetTextTime() noexcept;

		void _ChangeInternalText(std::string newText) noexcept;
		void _ChangeInternalPosition(glm::vec2 newPos) noexcept;
		void _ChangeInternalColour(ColourData newColour) noexcept;
		void _ChangeInternalFontSize(std::uint8_t newFontSize) noexcept;

		glm::vec2 GetPosition() const noexcept;
		int GetDisplayLength() const noexcept;
		std::string& GetText() noexcept;
		ColourData GetColour() const noexcept;
		std::uint8_t GetFontSize() const noexcept;
		float GetTime() const noexcept;
	private:
		std::string m_text;
		glm::vec2 m_pos = { 0.0f, 0.0f };
		float m_textTime = 0.0f;

		std::uint16_t m_displayLength = 0u;
		std::uint8_t m_fontSize = 16u;

		ColourData m_RGBColour = { 255u, 255u, 255u, 7u };
	public:
		TextType type = TextType::Default;
		std::uint8_t loggedErrors = false;
		GLuint vbo;
	};

	typedef ScreenText::TextType T_Type;

	static constexpr float hideTimer = 5.0f;
	static constexpr float spaceCharacterUnits = 2.0f;
	static constexpr float defaultFontSize = 12.0f;
	float characterSpacingUnits = 0.0f;
	float textWidth = 0.0f;
	float textHeight = 0.0f;

	TextRenderer() noexcept;
	void RenderText() const noexcept;

	ScreenText* CreateText(
		glm::vec2 pos, 
		std::string text, 
		T_Type textType = T_Type::Default, 
		std::uint8_t fontSize = static_cast<std::uint8_t>(defaultFontSize)
	) noexcept;
	void RecalculateAllText() noexcept;

	ScreenText* GetTextFromID(std::uint16_t id) noexcept;
	std::uint16_t GetIDFromText(ScreenText* screenText) const noexcept;

	void ChangeText(ScreenText* screenText, std::string newText, bool update = true) noexcept;
	void ChangePosition(ScreenText* screenText, glm::vec2 newPos, bool update = true) noexcept;
	void ChangeFontSize(ScreenText* screenText, std::uint8_t newFontSize, bool update = true) noexcept;
	void ChangeColour(ScreenText* screenText, ScreenText::ColourData newColour, bool update = true) noexcept;

	float GetCharScreenWidth(int charIndex, float fontMultiplier) const noexcept;

	void CheckTextStatus() noexcept;
	void RemoveText(std::uint16_t id) noexcept;

	~TextRenderer() noexcept;
private:
	// Characters sizes as seen in text texture (follows ASCII, starting after the 'space' character)
	static constexpr std::uint8_t m_charSizes[94] = {
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
		3, 1, 3, 6
	};

	float GetCharScreenWidth_M(int charIndex, float multiplier) const noexcept;

	std::unordered_map<std::uint16_t, ScreenText*> m_screenTexts;
	std::uint8_t m_textVAO, m_textVBO;

	void UpdateText(ScreenText* screenText) const noexcept;
	std::uint16_t GetNewID() noexcept;
};

#endif // _TEXTRENDERER_HEADER_
