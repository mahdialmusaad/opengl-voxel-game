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

	// Instanced attributes for individual text VBOs
	glEnableVertexAttribArray(0u);
	glVertexAttribDivisor(0u, 1u);

	// Shader buffer attributes
	glEnableVertexAttribArray(1u);
	glEnableVertexAttribArray(2u);
	glVertexAttribPointer(1u, 2, GL_FLOAT, GL_FALSE, sizeof(float[2]) + sizeof(std::uint32_t), nullptr);
	glVertexAttribIPointer(2u, 1, GL_UNSIGNED_INT, sizeof(float[2]) + sizeof(std::uint32_t), reinterpret_cast<const void*>(sizeof(float[2])));

	struct TextSSBOData {
		float positions[95]; // 94 unique characters + 1 for end of image
		float sizes[94]; // Floating point representation of each character's size
	} textSSBOData;
	
	textSSBOData.positions[94] = 1.0f; // End of image coordinate

	// Each character is seperated by a pixel on either side to ensure that parts 
	// don't get cut off due to precision errors in the texture coordinates.
	
	// Calculate size of a pixel on the font texture
	const float pixelSize = 1.0f / static_cast<float>(game.textTextureInfo.width);
	float currentImageOffset = pixelSize; // Current coordinate of character
	for (std::size_t i = 0u; i < sizeof(m_charSizes); ++i) {
		const float charSize = static_cast<float>(m_charSizes[i]); // Get character size as float
		textSSBOData.positions[i] = currentImageOffset; // Current position
		textSSBOData.sizes[i] = fmaxf(charSize, 1.4f); // Save floating point character size to the struct (minimum size to prevent weird appearance)
		currentImageOffset += pixelSize * (charSize + 2.0f); // Advance to next character (each seperated by 2 pixels)
	}

	// Send the pixel offsets of each letter in font texture to shader
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1u, OGL::CreateBuffer(GL_SHADER_STORAGE_BUFFER));
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(textSSBOData), &textSSBOData, 0u);

	TextFormat::log("Text renderer exit");
}

void TextRenderer::RenderText() const noexcept
{
	// Use correct buffers and the text shader
	game.shader.UseShader(Shader::ShaderID::Text);
	glBindVertexArray(m_textVAO);

	// No wireframe for text
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Render text if it is not marked as hidden
	for (const auto& [id, text] : m_screenTexts) {
		if (text->type == T_Type::Hidden) continue;
		glBindBuffer(GL_ARRAY_BUFFER, text->vbo);
		glVertexAttribIPointer(0u, 2, GL_UNSIGNED_INT, 0, nullptr);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, text->GetDisplayLength());
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
	for (const auto& [id, scrText] : m_screenTexts) if (scrText->vbo == screenText->vbo) return id;
	return 0u;
}

TextRenderer::ScreenText* TextRenderer::CreateText(glm::vec2 pos, std::string text, T_Type textType, std::uint8_t fontSize) noexcept
{
	std::uint16_t id = GetNewID(); // Create unique identifier for this text object
	TextFormat::log("Text creation, ID: " + std::to_string(id));

	// Create new text object and add to unordered map
	ScreenText *newScreenText = new ScreenText(pos, text, textType, fontSize);
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
void TextRenderer::ChangeFontSize(ScreenText* screenText, std::uint8_t newFontSize, bool update) noexcept
{
	screenText->_ChangeInternalFontSize(newFontSize);
	if (update) UpdateText(screenText);
}
void TextRenderer::ChangeColour(ScreenText* screenText, ScreenText::ColourData newColour, bool update) noexcept
{
	screenText->_ChangeInternalColour(newColour);
	if (update) UpdateText(screenText);
}

void TextRenderer::RemoveText(std::uint16_t id) noexcept
{
	// Clear up any memory used by the given text object
	const std::string idText = std::to_string(id);
	if (game.mainLoopActive) TextFormat::log("Text removed, ID: " + idText);

	ScreenText* screenText = GetTextFromID(id);
	if (screenText == nullptr) {
		TextFormat::log("Attempted to remove non-existent text ID " + idText);
		return;
	}

	glDeleteBuffers(1, &screenText->vbo);
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
		if (text->type == T_Type::Temporary && timePassed) toRemove.emplace_back(id);
		else if (text->type == T_Type::TemporaryShow && timePassed) text->type = T_Type::Hidden;
	}

	for (const std::uint16_t id : toRemove) RemoveText(id);
}

float TextRenderer::GetCharScreenWidth(int charIndex, float fontMultiplier) const noexcept
{
	// General width calculator, use in other files to determine the relative width of a character
	return (static_cast<float>(m_charSizes[charIndex]) * textWidth * fontMultiplier) + characterSpacingUnits;
}

float TextRenderer::GetCharScreenWidth_M(int charIndex, float multiplier) const noexcept
{
	// Used when updating text, as multiplying 'textWidth' and 'fontMultiplier' can be calculated beforehand
	return (static_cast<float>(m_charSizes[charIndex]) * multiplier) + characterSpacingUnits;
}

void TextRenderer::UpdateText(ScreenText *screenText) const noexcept
{
	// Bind buffers to update
	glBindVertexArray(m_textVAO);
	glBindBuffer(GL_ARRAY_BUFFER, screenText->vbo);
	glVertexAttribIPointer(0u, 2, GL_UNSIGNED_INT, sizeof(std::uint32_t[2]), nullptr); // Instanced text buffer attribute

	// Compressed text data
	const int displayLength = screenText->GetDisplayLength();
	if (displayLength == 0) return;
	std::uint32_t* textData = new std::uint32_t[displayLength * 2];
	
	// Data for second uint (colour and font size)
	const ScreenText::ColourData colours = screenText->GetColour();
	const std::uint8_t fontSize = screenText->GetFontSize();
	// Bits arrangement: FFFF FAAA BBBB BBBB GGGG GGGG RRRR RRRR
	const std::uint32_t second = 
		static_cast<std::uint32_t>(colours.x) +
		(static_cast<std::uint32_t>(colours.y) << 8u) + 
		(static_cast<std::uint32_t>(colours.z) << 16u) + 
		(static_cast<std::uint32_t>(colours.w) << 24u) +
		(static_cast<std::uint32_t>(fontSize) << 27u);

	// Starting text position (top-left corner)
	glm::vec2 pos = screenText->GetPosition();
	const float initialXPosition = pos.x;

	// Size multipliers - half the size as shaders use numbers -1 to 1 whereas here its stored as a float 0 to 1
	const float fontSizeMultiplier = (static_cast<float>(fontSize) / defaultFontSize) * 0.5f;
	const float textXMultiplier = textWidth * fontSizeMultiplier, lineOffset = textHeight * fontSizeMultiplier;

	// Error enum (use as bitmask)
	enum WarningBits : std::uint8_t { Warning_OOB = 1u, Warning_InvalidChar = 2u };
	
	int dataIndex = 0;
	for (const char& currentChar : screenText->GetText()) {
		// For a new line character, reset the X position offset and set the Y position lower
		if (currentChar == '\n') {
			pos.y -= lineOffset;
			pos.x = initialXPosition;
			continue;
		}
		// For a space, just increase the X position offset by an amount 
		else if (currentChar == ' ') {
			pos.x += (spaceCharacterUnits + characterSpacingUnits) * textXMultiplier;
			continue;
		}
		// Make sure only compatible text is shown
		else if (currentChar < ' ' || currentChar > '~') {
			if (!(screenText->loggedErrors & Warning_InvalidChar)) {
				screenText->loggedErrors |= Warning_InvalidChar;
				const int crnt = static_cast<int>(currentChar);
				const std::string err = fmt::format("ID: {}\nCharacter: {}\nPosition: {}, {}", GetIDFromText(screenText), crnt, pos.x, pos.y);
				TextFormat::warn(err, "Invalid text character in text object");
			}
			continue;
		}
		// Check for out of bounds text
		else if (pos.x < 0.0f || pos.x > 1.0f || pos.y < 0.0f || pos.y > 1.0f) {
			if (!(screenText->loggedErrors & Warning_OOB)) {
				const std::string err = fmt::format("ID: {}\nPosition: {}, {}", GetIDFromText(screenText), pos.x, pos.y);
				TextFormat::warn(err, "Attempted text rendering off-screen");
				screenText->loggedErrors |= Warning_OOB;
			}
			continue;
		}
		
		// Only characters after the space character (ASCII 32) are displayed
		const int charIndex = currentChar - 33;

		// Add compressed int data:
		// 0: TTTT TTTY YYYY YYYY YYYX XXXX XXXX XXXX
		// 1: FFFF FFFF BBBB BBBB GGGG GGGG RRRR RRRR
		const std::uint32_t letterData[2] = {
			(static_cast<std::uint32_t>(pos.x * 8191.0f)) +
			(static_cast<std::uint32_t>(pos.y * 4095.0f) << 13u) +
			(charIndex << 25u),
			second
		};

		std::memcpy(textData + dataIndex, letterData, sizeof(letterData));
		dataIndex += 2;

		// Make sure characters aren't rendered on top of each other; 
		// increase X position offset by the size of the character and a set amount
		pos.x += GetCharScreenWidth_M(charIndex, textXMultiplier);
	}
	
	// Buffer the text data to the GPU.
	glBufferData(GL_ARRAY_BUFFER, sizeof(std::uint32_t) * dataIndex, textData, GL_DYNAMIC_DRAW);
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
	std::uint8_t fontSize
) noexcept : 
	type(type),
	m_fontSize(fontSize)
{
	vbo = OGL::CreateBuffer(GL_ARRAY_BUFFER);
	_ChangeInternalText(text);
	_ChangeInternalPosition(pos);
	ResetTextTime();
}

// The weird setter names is a reminder to use the TextRenderer's update functions
// rather than changing them directly in the text object (lazy solution, I know)

// Internal setters
void TextRenderer::ScreenText::_ChangeInternalColour(ColourData newColour) noexcept
{ 
	m_RGBColour = newColour;
}
void TextRenderer::ScreenText::_ChangeInternalFontSize(std::uint8_t newFontSize) noexcept
{ 
	m_fontSize = newFontSize;
}
void TextRenderer::ScreenText::_ChangeInternalText(std::string newText) noexcept
{ 
	m_text = newText;
	// Update the 'display length' variable with how many *visible* characters are present (no spaces, newlines, etc)
	int len = 0;
	for (char x : m_text) if (x > ' ') ++len;
	m_displayLength = static_cast<std::uint16_t>(len);
}
void TextRenderer::ScreenText::ResetTextTime() noexcept
{ 
	m_textTime = static_cast<float>(glfwGetTime());
}
void TextRenderer::ScreenText::_ChangeInternalPosition(glm::vec2 newPos) noexcept
{ 
	m_pos = glm::vec2(newPos.x, 1.0f - newPos.y);
}

// Variable getters
int TextRenderer::ScreenText::GetDisplayLength() const noexcept { return static_cast<int>(m_displayLength); }
std::string& TextRenderer::ScreenText::GetText() noexcept { return m_text; }
TextRenderer::ScreenText::ColourData TextRenderer::ScreenText::GetColour() const noexcept { return m_RGBColour; }
std::uint8_t TextRenderer::ScreenText::GetFontSize() const noexcept { return m_fontSize; }
float TextRenderer::ScreenText::GetTime() const noexcept { return m_textTime; }
glm::vec2 TextRenderer::ScreenText::GetPosition() const noexcept { return m_pos; }
