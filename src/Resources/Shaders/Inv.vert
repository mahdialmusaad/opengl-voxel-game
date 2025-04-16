#version 430 core

uniform const vec2 texturePositions[][4] = {
	// { { TX }, { TY }, { WH }, { XY } }
	// Background
	{ vec2(0.04, 0.050), vec2(0.05, 0.060), vec2(2.00, 2.00), vec2(0.00, 0.00) },	// Textute index 0
	// Unequipped hotbar icon
	{ vec2(0.00, 0.500), vec2(0.00, 0.300), vec2(0.10, 0.10), vec2(0.55, 0.00) },	// Textute index 1
	// Equipped hotbar icon
	{ vec2(0.50, 1.000), vec2(0.00, 0.300), vec2(0.10, 0.10), vec2(0.55, 0.00) },	// Textute index 2
	// Inventory
	{ vec2(0.00, 1.000), vec2(0.90, 0.300), vec2(1.00, 1.00), vec2(0.50, 0.28) },	// Textute index 3
	// Crosshair
	{ vec2(0.00, 0.167), vec2(1.00, 0.900), vec2(0.02, 0.02), vec2(0.98, 0.98) }	// Textute index 4
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

layout (location = 0) in vec3 baseGUI;
layout (location = 1) in ivec2 baseIndexes;
layout (location = 2) in uvec2 data;
// YYYY YYYY YYYY YYYY XXXX XXXX XXXX XXXX
// WTTT TTTT TTTT TTTT TTTT TTTT TTTT TTTT

out vec2 TexCoord;
out flat int which;

void main()
{
	uint texID = data.y & 0x3FFFFFFF;
	float x = ((data.x & 0xFFFF) / 32767.0) - 1.0;
	float y = ((data.x >> 16) / 32767.0) - 1.0;
	
	if (data.y >> 31 == 1) {
		which = 1; // blocks

		gl_Position = vec4(
			x + 0.562 + (baseGUI.x * inventoryWidth * 0.0755),
			y + (baseGUI.y * inventoryHeight * 0.075) + 0.019, 
			0.0, 1.0
		);

		TexCoord = vec2(
			(baseGUI.x + texID) * blockTextureSize, 
			baseGUI.z
		);
	} else {
		which = 0; // inventory
		const vec2 texPositions[4] = texturePositions[texID];
		const vec2 WH = texPositions[2];
		const vec2 XY = texPositions[3];

		gl_Position = vec4(
			x + XY[0] + (baseGUI.x * WH[0] * inventoryWidth),
			y + XY[1] + (baseGUI.y * WH[1] * inventoryHeight), 
			0.0, 1.0
		);

		TexCoord = vec2(
			texPositions[0][baseIndexes.x], 
			texPositions[1][baseIndexes.y]
		);
	}
}
