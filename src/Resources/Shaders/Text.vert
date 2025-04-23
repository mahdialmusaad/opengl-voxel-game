#version 430 core

layout (location = 0) in uvec2 data;
// 0: 0TTT TTTT YYYY YYYY YYYY XXXX XXXX XXXX
// 1: FFFF FFFF BBBB BBBB GGGG GGGG RRRR RRRR
layout (location = 1) in vec2 baseGUI;

layout (std430, binding = 1) readonly buffer textureIndexes {
	float texturePositions[95];
	float textureSizes[94];
};

layout (std140, binding = 4) uniform GameSizes {
	float blockTextureSize;
	float inventoryTextureSize;
	float screenAspect;
	float textWidth;
	float textHeight;
	float inventoryWidth;
	float inventoryHeight;
};

out vec2 TexCoord;
out vec4 TextCol;

void main()
{
	float x = (data.x & 0xFFF) / 2047.5 - 1.0;
	float y = (data.x >> 12 & 0xFFF) / 2047.5 - 1.0;

	uint textindex = (data.x >> 24) & 0x7F;
	float fontSize = ((data.y >> 24) & 0xFF) / 16.0;
	float charSize = textureSizes[textindex] * fontSize;

	gl_Position = vec4(
		x + (baseGUI.x * textWidth * charSize), 
		y - (baseGUI.y * textHeight * fontSize), 
		0.0, 1.0
	);

	TexCoord = vec2(texturePositions[textindex + int(baseGUI.x)], baseGUI.y);
	TextCol = vec4(
		(data.y & 0xFF) / 255.0, 
		((data.y >> 8) & 0xFF) / 255.0, 
		((data.y >> 16) & 0xFF) / 255.0,
		1.0
	);
}
