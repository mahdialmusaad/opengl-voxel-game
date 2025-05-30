#include "TextRenderer.hpp"

TextRenderer::TextRenderer() noexcept
{
	TextFormat::log("Text renderer enter");

	// Setup text buffers to store instanced vertices
	m_textVAO = OGL::CreateVAO8();
	m_textVBO = OGL::CreateBuffer8(GL_ARRAY_BUFFER);

	// Default instanced data for a text quad
	struct TextRendererVertex {
		constexpr TextRendererVertex(float x, float y) : x(x), y(y), xInd(static_cast<std::uint32_t>(x)) {};
		float x, y;
		std::uint32_t xInd; 
	} constexpr quadVerts[4] = {
		{ 0.0f, 0.0f },
		{ 0.0f, 1.0f },
		{ 1.0f, 0.0f },
		{ 1.0f, 1.0f }
	};

	glBufferStorage(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, 0u);

	// Shader buffer attributes
	glEnableVertexAttribArray(0u);
	glEnableVertexAttribArray(1u);

	// Instanced attributes for individual character data
	glEnableVertexAttribArray(2u);
	glVertexAttribDivisor(2u, 1u);
	glEnableVertexAttribArray(3u);
	glVertexAttribDivisor(3u, 1u);

	// Non-instanced attribute (attrib 0+1) data stride and offset
	constexpr GLsizei stride = sizeof(float[2]) + sizeof(std::uint32_t);
	glVertexAttribPointer(0u, 2, GL_FLOAT, GL_FALSE, stride, nullptr);
	glVertexAttribIPointer(1u, 1, GL_UNSIGNED_INT, stride, reinterpret_cast<const void*>(sizeof(float[2])));

	// Calculate size of a pixel on the font texture
	const float pixelSize = 1.0f / static_cast<float>(game.textTextureInfo.width);

	float textUniformPositions[96]; // 94 unique characters + background texture + end of image
	textUniformPositions[95] = 1.0f; // Image end coordinate

	// Each character is seperated by a pixel on either side to ensure that parts 
	// don't get cut off due to precision errors in the texture coordinates.
	float currentImageOffset = pixelSize; // Current coordinate of character
	for (std::size_t i = 0u; i < sizeof(m_charSizes); ++i) {
		const float charSize = static_cast<float>(m_charSizes[i]); // Get character size as float
		textUniformPositions[i] = currentImageOffset; // Current position
		currentImageOffset += pixelSize * (charSize + 2.0f); // Advance to next character (each seperated by 2 pixels)
	}

	// Get location of positions array in shader and set uniform value (need to activate shader first)
	game.shader.UseShader(Shader::ShaderID::Text);
	texturePositionsLocation = game.shader.GetLocation(game.shader.ShaderFromID(Shader::ShaderID::Text), "texturePositions");
	glUniform1fv(texturePositionsLocation, sizeof(textUniformPositions) / sizeof(float), textUniformPositions);

	TextFormat::log("Text renderer exit");
}

void TextRenderer::RenderText() const noexcept
{
	// Use correct buffers and the text shader
	game.shader.UseShader(Shader::ShaderID::Text);
	glBindVertexArray(m_textVAO);

	// No wireframe for text
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Render text if it is not marked as hidden and has (visible) text
	for (const auto& [id, text] : m_screenTexts) {
		// Determine if the text is visible and has any text or background
		const int displayLength = text->GetDisplayLength();
		const int hasBackground = text->HasSettingEnabled(TS_Background);
		if (text->textType == TextType::Hidden || (displayLength == 0 && !hasBackground)) continue;

		// Bind current text object's VBO
		glBindBuffer(GL_ARRAY_BUFFER, text->GetVBO());

		// Set attribute layouts
		constexpr GLsizei stride = sizeof(float[4]) + sizeof(std::uint32_t[2]);
		glVertexAttribPointer(2u, 4, GL_FLOAT, GL_FALSE, stride, nullptr);
		glVertexAttribIPointer(3u, 2, GL_UNSIGNED_INT, stride, reinterpret_cast<const void*>(sizeof(float[4])));

		// Render text object (1 extra instance for background if enabled)
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (text->HasSettingEnabled(TS_Shadow) ? displayLength * 2 : displayLength) + hasBackground);
	}

	// Set wireframe mode to original
	if (game.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void TextRenderer::RecalculateAllText() noexcept
{
	// If global text variables (such as the line spacing) are changed,
	// all of the text objects need to be updated to use the new value.
	if (!m_screenTexts.size()) return;
	TextFormat::log("Calculate text");

	// Bind the text VAO so the correct buffers are updated
	glBindVertexArray(m_textVAO);

	// Update each text object
	for (const auto& [id, text] : m_screenTexts) UpdateText(text);
}

std::uint16_t TextRenderer::GetNewID() noexcept
{
	bool found = true;
	std::uint16_t id = 0u;

	// Random number generator
	std::mt19937 gen(std::random_device{}());
	std::uniform_int_distribution<std::uint16_t> dist(1, UINT16_MAX);
	
	// Repeatedly generates new random id until an unused one is found
	do {
		id = dist(gen);
		found = GetTextFromID(id) != nullptr;
	} while (found);

	return id;
}
TextRenderer::ScreenText* TextRenderer::GetTextFromID(std::uint16_t id) noexcept
{
	// Returns a text object if one is found with the given id
	const auto& textIterator = m_screenTexts.find(id);
	return textIterator != m_screenTexts.end() ? textIterator->second : nullptr;
}

std::uint16_t TextRenderer::GetIDFromText(ScreenText* screenText) const noexcept
{
	// Returns the ID of the given text object by
	// checking which VBO matches (unique per text object)
	const GLuint targetVBO = screenText->GetVBO();
	for (const auto& [id, scrText] : m_screenTexts) if (scrText->GetVBO() == targetVBO) return id;
	return 0u;
}

TextRenderer::ScreenText* TextRenderer::CreateText(
	glm::vec2 pos, 
	std::string text, 
	std::uint8_t settings,
	TextType textType, 
	std::uint8_t unitSize
) noexcept {
	// Create unique identifier for this text object
	std::uint16_t id = GetNewID();
	TextFormat::log("Text creation, ID: " + std::to_string(id));

	// Create new text object and add to unordered map
	ScreenText *newScreenText = new ScreenText(pos, text, textType, settings, unitSize);
	m_screenTexts.insert({ id, newScreenText });

	// Update the text buffers
	UpdateText(newScreenText);

	return newScreenText;
}

// Change a certain property of a text object and update the vertex data
void TextRenderer::ChangePosition(ScreenText* screenText, glm::vec2 newPos, bool update) noexcept
{
	screenText->_ChangeInternalPosition(newPos);
	if (update) UpdateText(screenText);
}
void TextRenderer::ChangeText(ScreenText* screenText, std::string newText, bool update) noexcept
{
	screenText->_ChangeInternalText(newText);
	if (update) UpdateText(screenText);
}
void TextRenderer::ChangeUnitSize(ScreenText* screenText, std::uint8_t newUnitSize, bool update) noexcept
{
	screenText->_ChangeInternalUnitSize(newUnitSize);
	if (update) UpdateText(screenText);
}
void TextRenderer::ChangeColour(ScreenText* screenText, ColourData newColour, bool update) noexcept
{
	screenText->_ChangeInternalColour(newColour);
	if (update) UpdateText(screenText);
}
void TextRenderer::ChangeSettings(ScreenText *screenText, std::uint8_t settings, bool update) noexcept
{
	screenText->_ChangeInternalSettings(settings);
	if (update) UpdateText(screenText);
}

void TextRenderer::RemoveText(std::uint16_t id) noexcept
{
	// Clear up any memory used by the given text object
	const std::string idText = std::to_string(id);
	if (game.mainLoopActive) TextFormat::log("Text removed, ID: " + idText);

	// Check if the given id corresponds to any text
	ScreenText* screenText = GetTextFromID(id);
	if (screenText == nullptr) {
		TextFormat::log("Attempted to remove non-existent text ID " + idText);
		return;
	}

	// Delete the text object to call destructor and remove from text object map
	m_screenTexts.erase(id);
	delete screenText;
}

void TextRenderer::CheckTextStatus() noexcept
{
	/*
		If an object has existed for longer than the global hide timer:
		* 'Temporary' type: Delete text object after time limit
		* 'TemporaryShow' type: Hide object after time limit
		* 'Default' type: Do nothing
	*/

	// Cannot remove text objects from the map during range for loop
	// so delete them seperately from the loop
	std::vector<std::uint16_t> toRemove;
	float currentTime = static_cast<float>(glfwGetTime());

	for (const auto& [id, text] : m_screenTexts) {
		bool timePassed = currentTime - text->GetTime() > hideTimer;
		if (text->textType == TextType::Temporary && timePassed) toRemove.emplace_back(id);
		else if (text->textType == TextType::TemporaryShow && timePassed) text->textType = TextType::Hidden;
	}

	for (const std::uint16_t id : toRemove) RemoveText(id);
}

float TextRenderer::GetCharScreenWidth(int charIndex, float unitMultiplier) const noexcept
{
	// Character width calculator excluding letter spacing
	return fmaxf(static_cast<float>(m_charSizes[charIndex]), 2.0f) * textWidth * unitMultiplier;
}

void TextRenderer::UpdateText(ScreenText *screenText) const noexcept
{
	// Bind buffers to update
	glBindVertexArray(m_textVAO);
	glBindBuffer(GL_ARRAY_BUFFER, screenText->GetVBO());
	
	// Save RGBA 0-255 values into one integer
	const ColourData colours = screenText->GetColour();
	// Bits arrangement: AAAA AAAA BBBB BBBB GGGG GGGG RRRR RRRR
	const std::uint32_t colourIntValue = 
		static_cast<std::uint32_t>(colours.x) +
		(static_cast<std::uint32_t>(colours.y) << 8u) + 
		(static_cast<std::uint32_t>(colours.z) << 16u) + 
		(static_cast<std::uint32_t>(colours.w) << 24u);

	const int hasBackground = screenText->HasSettingEnabled(TS_Background);
	const bool doShadowText = screenText->HasSettingEnabled(TS_Shadow);
	const int displayLength = screenText->GetDisplayLength();

	// Text data struct (vec3 + uvec2)
	struct TextObjectCharData {
		float x, y, w, h;
		std::uint32_t charIndex, col;
	} *textData = new TextObjectCharData[(doShadowText ? displayLength * 2 : displayLength) + hasBackground];

	// Starting text position (top-left corner), multiply by 2 and sub 1 to use range [-1, 1] for shader
	glm::vec2 pos = { (screenText->GetPosition().x * 2.0f) - 1.0f, (screenText->GetPosition().y * 2.0f) - 1.0f };

	const glm::vec2 initialPosition = pos; // Beginning position for new lines and background
	glm::vec2 maxPos = pos; // Bottom-right corner (for background)
	
	// Size multipliers
	const std::uint8_t unitSize = screenText->GetUnitSize();
	const float unitSizeMultiplier = static_cast<float>(unitSize) / defaultUnitSize;
	const float spaceOffset = spaceCharacterSize * unitSizeMultiplier;
	const float characterHeight = unitSizeMultiplier * textHeight;

	// Update maximum X and/or Y position when changing
	const auto& UpdateMaxPosX = [&]() { if (pos.x > maxPos.x) maxPos.x = pos.x; };
	const auto& UpdateMaxPos = [&]() { UpdateMaxPosX(); if (pos.y < maxPos.y) maxPos.y = pos.y; };

	int dataIndex = hasBackground;
	for (const char& currentChar : screenText->GetText()) {
		// For a new line character, reset the X position offset and set the Y position lower
		if (currentChar == '\n') {
			pos.y -= characterHeight;
			pos.x = initialPosition.x;
			UpdateMaxPos();
			continue;
		}
		// For a space, just increase the X position offset by an amount 
		else if (currentChar == ' ') {
			pos.x += spaceOffset;
			UpdateMaxPosX();
			continue;
		}
		// Make sure only compatible text is shown
		else if (currentChar < ' ' || currentChar > '~') {
			const int crnt = static_cast<int>(currentChar);
			const std::string err = fmt::format("ID: {}\nCharacter: {}\nPosition: {}, {}", GetIDFromText(screenText), crnt, pos.x, pos.y);
			TextFormat::warn(err, "Invalid text character in text object");
			continue;
		}
		
		// Only characters after the space character (ASCII 32) are displayed
		const int charIndex = currentChar - 33;

		// Get display width of character
		const float charWidth = GetCharScreenWidth(charIndex, unitSizeMultiplier);

		// Create new character object with current values
		const TextObjectCharData letterData {
			pos.x, pos.y, charWidth, characterHeight,
			static_cast<std::uint32_t>(charIndex), colourIntValue
		};

		// Create a black and slightly offset copy for shadow text rendered underneath/before actual character
		if (doShadowText) {
			constexpr std::uint32_t blackColourValue = 255u << 24u;
			constexpr float shadowOffset = 0.005f;
			const TextObjectCharData shadowLetterData = {
				pos.x + shadowOffset, pos.y - shadowOffset, charWidth, characterHeight,
				letterData.charIndex, blackColourValue
			};
			// Add shadow character to array
			std::memcpy(textData + (dataIndex++), &shadowLetterData, sizeof(shadowLetterData));
		}
		
		// Add character object to array
		std::memcpy(textData + (dataIndex++), &letterData, sizeof(letterData));

		// Advance in X axis by the character width and letter spacing
		pos.x += charWidth + characterSpacingUnits;
		UpdateMaxPosX();
	}

	// Add background if it was enabled - use initial and ending position to determine size
	// The background texture is made up of an entire white column in the texture so it appears as a box
	if (hasBackground) {
		// Default background colour
		constexpr std::uint32_t colour = 100u;
		constexpr std::uint32_t backgroundRGBA = colour + (colour << 8u) + (colour << 16u) + (colour << 24u);
		constexpr std::uint32_t backgroundChar = static_cast<std::uint32_t>(sizeof(m_charSizes)) - 1u;

		const int fullXBackground = screenText->HasSettingEnabled(TS_BackgroundFullX);
		const int fullYBackground = screenText->HasSettingEnabled(TS_BackgroundFullY);
		
		// Start of background lines up exactly with text, increase width a bit for better appearance
		const float startX = fullXBackground ? -1.0f : initialPosition.x - 0.005f;
		const float startY = fullYBackground ? -1.0f : initialPosition.y;

		// Background X and Y sizes
		const float sizeX = fullXBackground ? 2.0f : maxPos.x - startX;
		const float sizeY = fullYBackground ? 2.0f : (initialPosition.y - maxPos.y) + characterHeight;

		textData[0] = {
			startX, startY, sizeX, sizeY,
			backgroundChar, backgroundRGBA
		};
	}
	
	// Buffer the text data to the GPU.
	glBufferData(GL_ARRAY_BUFFER, sizeof(TextObjectCharData) * dataIndex, textData, GL_DYNAMIC_DRAW);
	// Array was created with 'new', so make sure to free up the memory
	delete[] textData;
}

TextRenderer::~TextRenderer() noexcept
{
	// Delete all text objects from their ids (cannot loop over and delete at the same time)
	std::vector<std::uint16_t> ids;
	ids.reserve(m_screenTexts.size());
	for (const auto& [id, text] : m_screenTexts) ids.emplace_back(id);
	for (const std::uint16_t& id : ids) RemoveText(id);
	
	// Bind VAO to retrieve correct buffer IDs
	glBindVertexArray(m_textVAO);

	// Get text SSBO ID
	GLint textSSBO;
	glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &textSSBO);
	
	// Delete renderer buffers
	const GLuint deleteBuffers[] = { static_cast<GLuint>(textSSBO), static_cast<GLuint>(m_textVBO) };
	glDeleteBuffers(sizeof(deleteBuffers) / sizeof(GLuint), deleteBuffers);

	// Delete renderer VAO
	const GLuint deleteVAO = static_cast<GLuint>(m_textVAO);
	glDeleteVertexArrays(1, &deleteVAO);
}

TextRenderer::ScreenText::ScreenText(
	glm::vec2 pos, 
	std::string text, 
	TextType type,
	std::uint8_t settings,
	std::uint8_t unitSize
) noexcept : m_settings(settings), m_unitSize(unitSize), textType(type) 
{
	m_vbo = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	_ChangeInternalText(text);
	_ChangeInternalPosition(pos);
	ResetTextTime();
}
TextRenderer::ScreenText::~ScreenText() noexcept { glDeleteBuffers(1, &m_vbo); }

GLuint TextRenderer::ScreenText::GetVBO() const noexcept { return m_vbo; }
void TextRenderer::ScreenText::ResetTextTime() noexcept { m_textTime = static_cast<float>(glfwGetTime()); }

// The weird setter names is a reminder to use the TextRenderer's update functions
// rather than changing them directly in the text object (lazy solution, I know)

// Internal setters
void TextRenderer::ScreenText::_ChangeInternalText(std::string newText) noexcept
{
	// Get length of text, excluding control/non-standard characters
	m_text = newText;
	m_displayLength = 0u;
	for (char x : newText) if (x > ' ' && x <= '~') ++m_displayLength;
}

void TextRenderer::ScreenText::_ChangeInternalPosition(glm::vec2 newPos) noexcept { m_pos = glm::vec2(newPos.x, 1.0f - newPos.y); }
void TextRenderer::ScreenText::_ChangeInternalColour(ColourData newColour) noexcept { m_RGBColour = newColour; }
void TextRenderer::ScreenText::_ChangeInternalSettings(std::uint8_t settings) noexcept { m_settings = settings; }
void TextRenderer::ScreenText::_ChangeInternalUnitSize(std::uint8_t newUnitSize) noexcept { m_unitSize = newUnitSize; }

// Variable getters
TextRenderer::ColourData TextRenderer::ScreenText::GetColour() const noexcept { return m_RGBColour; }
glm::vec2 TextRenderer::ScreenText::GetPosition() const noexcept { return m_pos; }
std::string& TextRenderer::ScreenText::GetText() noexcept { return m_text; }
std::uint8_t TextRenderer::ScreenText::GetUnitSize() const noexcept { return m_unitSize; }
std::uint8_t TextRenderer::ScreenText::GetSettings() const noexcept { return m_settings; }

float TextRenderer::ScreenText::GetTime() const noexcept { return m_textTime; }
int TextRenderer::ScreenText::GetDisplayLength() const noexcept { return static_cast<int>(m_displayLength); }
int TextRenderer::ScreenText::HasSettingEnabled(TextSettings setting) const noexcept { return (m_settings & setting) == setting; }
