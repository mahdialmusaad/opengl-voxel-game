#version 430 core

uniform vec4 texturePositions[5] = { 
    // vec4( X1, X2 - X1, Y1, Y2 - Y1 )
    vec4( 0.04, 0.01, 0.05, 0.01 ), // Grey background
    vec4( 0.00, 0.50, 0.00, 0.30 ), // Unequipped slot
    vec4( 0.50, 0.50, 0.00, 0.30 ), // Equipped slot
    vec4( 0.00, 1.00, 0.30, 0.60 ), // Inventory box
    vec4( 0.00, 0.20, 0.90, 0.10 ), // Center crosshair
};

layout (std140, binding = 4) uniform GameSizes {
	float blockTextureSize;
	float inventoryTextureSize;
};

layout (location = 0) in vec4 d1; // X, Y, W, H
layout (location = 1) in uint d2; // Texture + type
layout (location = 2) in vec3 baseGUI;

out vec2 TexCoord;
out flat int which;

void main()
{
	const uint texID = d2 >> 1;
	
	if ((d2 & 1) == 1) {
		which = 1;
		TexCoord = vec2((baseGUI.x + float(texID)) * blockTextureSize, baseGUI.z);
	} else {
        which = 0;
        const vec4 texPositions = texturePositions[texID];
		TexCoord = vec2(
			(baseGUI.x * texPositions.y) + texPositions.x, 
			(baseGUI.y * texPositions.w) + texPositions.z
		);
	}

    gl_Position = vec4(
		d1.x + (baseGUI.x * d1.z),
		d1.y + (baseGUI.y * d1.w), 
		0.0, 1.0
	);
}
