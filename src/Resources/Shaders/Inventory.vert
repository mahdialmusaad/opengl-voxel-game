#version 430 core

uniform vec4 texturePositions[5] = { 
    // vec4( X1, X2 - X1, Y1, Y2 - Y1 )
    vec4( 0.04, 0.01, 0.05, 0.01 ), // Grey background
    vec4( 0.00, 0.50, 0.00, 0.30 ), // Unequipped slot
    vec4( 0.50, 0.50, 0.00, 0.30 ), // Equipped slot
    vec4( 0.00, 1.00, 0.30, 0.60 ), // Inventory box
    vec4( 0.00, 0.2, 0.90, 0.10 ), // Center crosshair
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

layout (location = 0) in uvec2 data;
// YYYY YYYY YYYY YYYY XXXX XXXX XXXX XXXX
// BTTT TTTT TTTT TTTT HHHH HHHH WWWW WWWW
layout (location = 1) in vec3 baseGUI;

out vec2 TexCoord;
out flat int which;

void main()
{
	const float x = ((data.x & 0xFFFF) / 32767.5) - 1.0;
	const float y = ((data.x >> 16) / 32767.5) - 1.0;

    const float w = ((data.y & 0xFF) / 127.0);
    const float h = (((data.y >> 8) & 0xFF) / 127.0);

	const uint texID = (data.y >> 16) & 0x7FFF;
	
	if (data.y >> 31u == 1u) {
		which = 1;
		TexCoord = vec2(
			(baseGUI.x + float(texID)) * blockTextureSize,
			baseGUI.z
		);
	} else {
        which = 0;
        const vec4 texPositions = texturePositions[texID];
		TexCoord = vec2(
			(baseGUI.x * texPositions.y) + texPositions.x, 
			(baseGUI.y * texPositions.w) + texPositions.z
		);
	}

    gl_Position = vec4(
		x + (baseGUI.x * inventoryWidth * w),
		y + (baseGUI.y * inventoryHeight * h), 
		0.0, 1.0
	);
}
