#include "TextRenderer.hpp"

TextRenderer::TextRenderer() noexcept
{
	// Setup text buffers to store instanced vertices
	m_textVAO = OGL::CreateVAO();
	m_textVBO = OGL::CreateBuffer(GL_ARRAY_BUFFER);

	// Default instanced data for a text quad
	struct TextRendererVertex {
		TextRendererVertex(float x, float y) noexcept : x(x), y(y), xInd(static_cast<std::uint32_t>(x)) {};
		float x, y;
		std::uint32_t xInd; 
	} const quadVerts[4] = {
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
	glVertexAttribPointer(0u, 2, GL_FLOAT, GL_FALSE, sizeof(float[2]) + sizeof(std::uint32_t), nullptr);
	glVertexAttribIPointer(1u, 1, GL_UNSIGNED_INT, sizeof(float[2]) + sizeof(std::uint32_t), reinterpret_cast<const void*>(sizeof(float[2])));

	// Update text shader uniform values with texture positions for each character
	UpdateShaderUniform();
}

void TextRenderer::UpdateShaderUniform() noexcept
{
	// Calculate size of a pixel on the font texture
	const double pixelSize = 1.0 / static_cast<double>(game.textTextureInfo.width);

	// 95 unique characters (2 each for start and end coordinate) + end of image value
	// ** Shader uniform size should match --> Text.vert **
	const int numCharsTexture = static_cast<int>(sizeof(game.constants.charSizes));
	const int positionDataAmnt = (numCharsTexture * 2) + 1;

	float textUniformPositions[positionDataAmnt]; // Start and end positions of characters in the image
	
	// Each character is seperated by a pixel on either side in the image texture (= 2 pixels between characters)
	float currentImageOffset = pixelSize; // Current coordinate of character (1 pixel gap to first character)
	for (int i = 0, n = 0; i < numCharsTexture; ++i) {
		const float charSize = static_cast<float>(game.constants.charSizes[i]); // Get character size as decimal value
		textUniformPositions[n++] = currentImageOffset; // Set start position of character texture
		textUniformPositions[n++] = currentImageOffset + (pixelSize * charSize); // Set end position of character texture
		currentImageOffset += pixelSize * (charSize + 2.0f); // Advance to next character (each separated by 2 pixels)
	}
	
	textUniformPositions[positionDataAmnt - 1] = 1.0f; // Texture end coordinate

	// Get location of positions array in shader and set uniform value
	texturePositionsLocation = game.shaders.programs.text.GetLocation("texturePositions");
	glUniform1fv(texturePositionsLocation, positionDataAmnt, textUniformPositions);
}

void TextRenderer::RenderText(bool inventoryStatus) const noexcept
{
	// Render text if the conditions (determined by text settings, background, text size, etc) are valid
	for (auto it = m_screenTexts.begin(); it != m_screenTexts.end(); ++it) RenderText(it->second, it == m_screenTexts.begin(), inventoryStatus);
}

void TextRenderer::RenderText(ScreenText *screenText, bool useShader, bool inventoryOpen) const noexcept
{
	// Setup shader if needed
	if (useShader) {
		game.shaders.programs.text.Use();
		glBindVertexArray(m_textVAO);
	}

	// Check for visibility settings when inventory is open
	bool showWhenInventory = screenText->HasSettingEnabled(TS_InventoryVisible);
	if ((!inventoryOpen && showWhenInventory) || (inventoryOpen && !showWhenInventory)) return;

	// Hide if the text is marked as debug and debug mode is not active
	if (!game.debugText && screenText->HasSettingEnabled(TS_DebugText)) return;
	
	// Determine if the text is visible and has any text or background
	const int displayLength = screenText->GetDisplayLength();
	const int hasBackground = screenText->HasSettingEnabled(TS_Background);
	if (screenText->textType == TextType::Hidden || (displayLength == 0 && !hasBackground)) return;

	// Bind current text object's VBO
	glBindBuffer(GL_ARRAY_BUFFER, screenText->GetVBO());

	// Set attribute layouts
	glVertexAttribPointer(2u, 4, GL_FLOAT, GL_FALSE, sizeof(float[4]) + sizeof(std::uint32_t[2]), nullptr);
	glVertexAttribIPointer(3u, 2, GL_UNSIGNED_INT, sizeof(float[4]) + sizeof(std::uint32_t[2]), reinterpret_cast<const void*>(sizeof(float[4])));

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // No wireframe for text
	
	// Render text object (1 extra instance for background if enabled)
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (displayLength * (screenText->HasSettingEnabled(TS_Shadow) + 1)) + hasBackground);

	if (game.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Set the wireframe mode to original
}

void TextRenderer::RecalculateAllText() noexcept
{
	// If global text variables (such as text width) are changed,
	// all of the text objects need to be updated to use the new value(s).
	if (!m_screenTexts.size()) return; // Check if there is any text in the first place

	glBindVertexArray(m_textVAO); // Bind the text VAO so the correct buffers are updated
	for (const auto &it : m_screenTexts) UpdateText(it.second); // Update each text object
}

std::uint16_t TextRenderer::GetNewID() noexcept
{
	// Random number generator
	std::mt19937 gen(std::random_device{}());
	std::uniform_int_distribution<std::uint16_t> dist(static_cast<std::uint16_t>(1u), static_cast<std::uint16_t>(0xFFFFu));
	
	// Repeatedly generates new random id until an unused one is found
	bool found;
	std::uint16_t id;
	do {
		id = dist(gen);
		found = GetTextFromID(id);
	} while (found);

	return id;
}
TextRenderer::ScreenText *TextRenderer::GetTextFromID(std::uint16_t id) noexcept
{
	// Returns a text object if one is found with the given id
	const auto &textIterator = m_screenTexts.find(id);
	return textIterator != m_screenTexts.end() ? textIterator->second : nullptr;
}

std::uint16_t TextRenderer::GetIDFromText(ScreenText *screenText) const noexcept
{
	// Returns the ID of the given text object by
	// checking which VBO matches (unique per text object)
	const GLuint targetVBO = screenText->GetVBO();
	for (const auto &it : m_screenTexts) if (it.second->GetVBO() == targetVBO) return it.first;
	return std::uint16_t{};
}

TextRenderer::ScreenText *TextRenderer::CreateText(
	glm::vec2 pos, 
	std::string text, 
	unsigned settings,
	unsigned unitSize,
	TextType textType,
	bool update
) noexcept {
	// Create unique identifier for this text object
	std::uint16_t id = GetNewID();

	// Create new text object and add to unordered map
	ScreenText *newScreenText = new ScreenText(pos, text, textType, settings, unitSize);
	m_screenTexts.insert({ id, newScreenText });

	// Update the text buffers
	if (update) UpdateText(newScreenText);

	return newScreenText;
}

TextRenderer::ScreenText *TextRenderer::CreateText(ScreenText *original, bool update)
{
	// Create a text object using another for values - copy of text
	return CreateText(original->GetPosition(), original->GetText(), original->GetUnitSize(), original->GetSettings(), original->textType, update);
}

// Change a certain property of a text object and update the vertex data
void TextRenderer::ChangePosition(ScreenText *screenText, glm::vec2 newPos, bool update) noexcept
{
	screenText->_ChangeInternalPosition(newPos);
	if (update) UpdateText(screenText);
}
void TextRenderer::ChangeText(ScreenText *screenText, std::string newText, bool update) noexcept
{
	screenText->_ChangeInternalText(newText);
	if (update) UpdateText(screenText);
}
void TextRenderer::ChangeUnitSize(ScreenText *screenText, std::uint8_t newUnitSize, bool update) noexcept
{
	screenText->_ChangeInternalUnitSize(newUnitSize);
	if (update) UpdateText(screenText);
}
void TextRenderer::ChangeColour(ScreenText *screenText, ColourData newColour, bool update) noexcept
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
	const std::string idText = fmt::to_string(id);
	TextFormat::log("Text removed, ID: " + idText);

	// Check if the given id corresponds to any text
	ScreenText *screenText = GetTextFromID(id);
	if (!screenText) {
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

	for (const auto &it : m_screenTexts) {
		ScreenText *text = it.second;
		bool timePassed = currentTime - text->GetTime() > 5.0f;
		if (text->textType == TextType::Temporary && timePassed) toRemove.emplace_back(it.first);
		else if (text->textType == TextType::TemporaryShow && timePassed) text->textType = TextType::Hidden;
	}

	for (const std::uint16_t id : toRemove) RemoveText(id);
}

float TextRenderer::GetUnitSizeMultiplier(std::uint8_t unitSize) const noexcept
{
	// Returns the float multiplier for a given unit size
	return static_cast<float>(unitSize) / static_cast<float>(defaultUnitSize);
}

float TextRenderer::GetCharScreenWidth(int charIndex, float unitMultiplier) const noexcept
{
	// Returns the given character's text width (excluding letter spacing)
	return static_cast<float>(game.constants.charSizes[charIndex]) * textWidth * unitMultiplier;
}

float TextRenderer::GetTextWidth(const std::string &text, std::uint8_t unitSize) const noexcept
{
	// Get relative size of character spacing and space character
	const float unitMultiplier = GetUnitSizeMultiplier(unitSize);
	const float spaceWidth = spaceCharacterSize * unitMultiplier, spacing = characterSpacingSize * unitMultiplier;
	float lineWidthSum = 0.0f; // Current width of line
	float largestLineWidth = 0.0f; // Largest recorded width

	const auto CheckText = [&](int c) {
		switch (c) {
			// Space character - just add width of a space
			case ' ':
				lineWidthSum += spaceWidth;
				break;
			// New line - Check if current line was largest and reset line width counter
			case '\n':
				largestLineWidth = fmaxf(largestLineWidth, lineWidthSum);
				lineWidthSum = 0.0f;
				break;
			// Any other character - add the size of the character as well as character spacing
			default:
				lineWidthSum += GetCharScreenWidth(c - 33, unitMultiplier) + spacing;
				break;
		}
	};

	// Calculate width of each line to determine the maximum width of the given string
	for (const char &c : text) CheckText(static_cast<int>(c));
	CheckText(static_cast<int>('\n')); // Include last line in comparison

	// Return width of largest line
	return largestLineWidth;
}

float TextRenderer::GetTextHeight(ScreenText *screenText) const noexcept
{
	const std::string &text = screenText->GetText(); // Text to calculate height of
	return GetTextHeight(screenText->GetUnitSize(), TextFormat::numChar(text, '\n') + 1); // Add extra to include single line text
}

float TextRenderer::GetTextHeight(std::uint8_t fontSize, int lines) const noexcept
{
	const float lineHeight = GetUnitSizeMultiplier(fontSize) * textHeight; // Get line height of text
	return lineHeight * static_cast<float>(lines); // Return line height multiplied by new line occurences
}

float TextRenderer::GetRelativeTextYPos(TextRenderer::ScreenText *sct, int linesOverride) noexcept
{
	// Get Y position just below the given text object -
	// an override on the number of lines can be given as well.
	return sct->GetPosition().y - (linesOverride == -1 ? GetTextHeight(sct) : GetTextHeight(sct->GetUnitSize(), linesOverride)) - 0.01f;
}

void TextRenderer::FormatChatText(std::string &text) const noexcept
{
	const std::size_t one = static_cast<std::size_t>(1u);
	const std::size_t maxCharLine = static_cast<std::size_t>(maxChatLineChars);
	std::size_t spaceIndex{};

	// Traverse through each word of the text, replacing spaces with new lines so each line
	// doesn't go beyond the line character limit. If a single word is larger than this limit,
	// split it into multiple lines so it fits within this limit.
	// Also respects previous new lines present in the text (to an extent?)
	
	bool loop = true;
	while (loop) {
		std::size_t nextSpaceIndex = text.find(' ', spaceIndex);
		loop = nextSpaceIndex != std::string::npos;
		if (!loop) nextSpaceIndex = text.size();

		if (nextSpaceIndex - spaceIndex > maxCharLine) {
			spaceIndex += maxCharLine;
			for (; spaceIndex < nextSpaceIndex; spaceIndex += maxCharLine) text.insert(spaceIndex, one, '\n');
		}
		else if (nextSpaceIndex - text.find_last_of('\n', nextSpaceIndex) > maxCharLine) text.insert(spaceIndex, one, '\n');

		spaceIndex = ++nextSpaceIndex;
	}
}

void TextRenderer::UpdateText(ScreenText *screenText) const noexcept
{
	game.perfs.textUpdate.Start();

	// Ensure no invalid characters are present
	const std::string &text = screenText->GetText();
	if (std::find_if(text.begin(), text.end(), [&](const char &c) { return (c < ' ' || c > '~') && c != '\n'; }) != text.end()) {
		TextFormat::warn(fmt::format("Invalid text found in text object {}. Update cancelled.", screenText->GetVBO()), "Invalid text");
		return;
	}

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

	// Starting text position (top-left corner)
	glm::vec2 pos = screenText->GetPosition();

	const glm::vec2 initialPosition = pos; // Beginning position for new lines and background
	glm::vec2 maxPos = pos; // Bottom-right corner (for background)
	
	// Size multipliers
	const std::uint8_t unitSize = screenText->GetUnitSize();
	const float unitSizeMultiplier = GetUnitSizeMultiplier(unitSize);
	const float spaceOffset = spaceCharacterSize * unitSizeMultiplier;
	const float characterHeight = unitSizeMultiplier * textHeight;
	const float spacing = characterSpacingSize * unitSizeMultiplier;

	// Update maximum X and/or Y position when changing
	const auto &UpdateMaxPosX = [&]() { if (pos.x > maxPos.x) maxPos.x = pos.x; };
	const auto &UpdateMaxPos = [&]() { UpdateMaxPosX(); if (pos.y < maxPos.y) maxPos.y = pos.y; };

	// Text data struct (vec3 + uvec2)
	struct TextObjectCharData {
		float x, y, w, h;
		std::uint32_t charIndex, col;
	} *textData = new TextObjectCharData[(doShadowText ? displayLength * 2 : displayLength) + hasBackground];

	int dataIndex = hasBackground;
	for (const char &currentChar : text) {
		const int charValue = static_cast<int>(currentChar);
		// For a new line character, reset the X position offset and set the Y position lower
		if (charValue == static_cast<int>('\n')) {
			pos.y -= characterHeight;
			pos.x = initialPosition.x;
			UpdateMaxPos();
			continue;
		}
		// For a space, just increase the X position offset by an amount 
		else if (currentChar == static_cast<int>(' ')) {
			pos.x += spaceOffset;
			UpdateMaxPosX();
			continue;
		}
		
		// Only characters after the space character (ASCII 32) are displayed
		const int charIndex = charValue - 33;

		// Get display width of character
		const float charWidth = GetCharScreenWidth(charIndex, unitSizeMultiplier);

		// Create new character object with current values
		const TextObjectCharData letterData {
			pos.x, pos.y, charWidth, characterHeight,
			static_cast<std::uint32_t>(charIndex) << static_cast<std::uint32_t>(1u), colourIntValue
		};

		// Create a black and slightly offset copy for shadow text rendered underneath/before actual character
		if (doShadowText) {
			const std::uint32_t blackColourValue = 255u << 24u;
			const TextObjectCharData shadowLetterData = {
				pos.x + (textWidth * 0.5f), 
				pos.y - (characterHeight / static_cast<float>(game.textTextureInfo.height)), charWidth, characterHeight,
				letterData.charIndex, blackColourValue
			};
			// Add shadow character to array
			std::memcpy(textData + (dataIndex++), &shadowLetterData, sizeof(shadowLetterData));
		}

		// Add character object to array
		std::memcpy(textData + (dataIndex++), &letterData, sizeof(letterData));
		
		// Advance in X axis by the character width and letter spacing
		pos.x += charWidth + spacing;
		UpdateMaxPosX();
	}

	// Add background if it was enabled - use initial and ending position to determine size
	// The background texture is made up of an entire white column in the texture so it appears as a box
	if (hasBackground) {
		// Default background colour
		const std::uint32_t colour = 100u;
		const std::uint32_t backgroundRGBA = colour + (colour << 8u) + (colour << 16u) + (colour << 24u);
		const std::uint32_t backgroundChar = static_cast<std::uint32_t>((static_cast<int>(sizeof(game.constants.charSizes)) - 1) * 2);

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
	
	glBufferData(GL_ARRAY_BUFFER, sizeof(TextObjectCharData) * dataIndex, textData, GL_DYNAMIC_DRAW); // Buffer the text data to the GPU
	delete[] textData; // Free up allocated memory

	game.perfs.textUpdate.End();
}

TextRenderer::~TextRenderer() noexcept
{
	// Delete all text objects
	for (const auto &it : m_screenTexts) delete it.second;
	
	// Bind VAO to retrieve correct buffer IDs
	glBindVertexArray(m_textVAO);

	// Get text SSBO ID
	GLint textSSBO;
	glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &textSSBO);
	
	// Delete renderer buffers
	const GLuint deleteBuffers[] = { static_cast<GLuint>(textSSBO), static_cast<GLuint>(m_textVBO) };
	glDeleteBuffers(static_cast<int>(Math::size(deleteBuffers)), deleteBuffers);

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

void TextRenderer::ScreenText::_ChangeInternalPosition(glm::vec2 newPos) noexcept { m_pos = newPos; }
void TextRenderer::ScreenText::_ChangeInternalColour(ColourData newColour) noexcept { m_RGBColour = newColour; }
void TextRenderer::ScreenText::_ChangeInternalSettings(std::uint8_t settings) noexcept { m_settings = settings; }
void TextRenderer::ScreenText::_ChangeInternalUnitSize(std::uint8_t newUnitSize) noexcept { m_unitSize = newUnitSize; }

// Variable getters
TextRenderer::ColourData TextRenderer::ScreenText::GetColour() const noexcept { return m_RGBColour; }
glm::vec2 TextRenderer::ScreenText::GetPosition() const noexcept { return m_pos; }
const std::string &TextRenderer::ScreenText::GetText() const noexcept { return m_text; }
std::uint8_t TextRenderer::ScreenText::GetUnitSize() const noexcept { return m_unitSize; }
std::uint8_t TextRenderer::ScreenText::GetSettings() const noexcept { return m_settings; }

float TextRenderer::ScreenText::GetTime() const noexcept { return m_textTime; }
int TextRenderer::ScreenText::GetDisplayLength() const noexcept { return static_cast<int>(m_displayLength); }
int TextRenderer::ScreenText::HasSettingEnabled(TextSettings setting) const noexcept { return (m_settings & setting) == setting; }
