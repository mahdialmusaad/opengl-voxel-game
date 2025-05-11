#version 430 core

layout (location = 0) in uvec2 data;
// 0: TTTT TTTY YYYY YYYY YYYX XXXX XXXX XXXX
// 1: FFFF FAAA BBBB BBBB GGGG GGGG RRRR RRRR
layout (location = 1) in vec2 baseGUI;
layout (location = 2) in uint baseIndex;

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
	const float x = (data.x & 0x1FFF) / 4095.5 - 1.0;
	const float y = (data.x >> 13 & 0xFFF) / 2047.5 - 1.0;

	const uint textindex = (data.x >> 25) & 0x7F;
	const float fontSize = ((data.y >> 27) & 0x1F) / 12.0;
	const float charSize = textureSizes[textindex];

	gl_Position = vec4(
		x + (baseGUI.x * textWidth * fontSize * charSize),
		y - (baseGUI.y * textHeight * fontSize),
		0.0, 1.0
	);

	TexCoord = vec2(texturePositions[textindex + baseIndex], baseGUI.y);
	TextCol = vec4(
		(data.y & 0xFF) / 255.0, 
		((data.y >> 8) & 0xFF) / 255.0, 
		((data.y >> 16) & 0xFF) / 255.0,
		((data.y >> 24) & 0x7) / 7.0
	);
}
